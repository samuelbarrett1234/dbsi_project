#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <algorithm>
#include "dbsi_assert.h"
#include "dbsi_dictionary.h"
#include "dbsi_rdf_index.h"
#include "dbsi_turtle.h"
#include "dbsi_query.h"
#include "dbsi_dictionary_utils.h"
#include "dbsi_nlj.h"


using namespace dbsi;


/*
* This class is a bit annoying to have to implement,
* but it serves a single niche purpose, later in this
* file. If this is your first time reading the file,
* ignore this class.
* 
* What it does: takes an iterator over values of type
* U, but ignores those values, and always returns a
* default-constructed instance of a T.
* It is valid precisely when the given input U-iterator
* is valid.
* 
* What this is used for: when performing a selection
* over the entire DB, we use the RDF index's `full_scan`
* method, which returns an iterator over coded triples.
* However, our function `QueryApplication::evaluate_patterns`
* below returns iterators over `VarMap`s. But, since we
* don't care about the return results (*), we can just
* default construct `VarMap` for each coded triple in the
* DB.
* 
* (*) Why we don't care about the return results: because
* the query is of the form
* SELECT/COUNT ?X1 ... ?Xn WHERE { }
* so the variables are not bound to anything!
*/
template<typename T, typename U>
class NullIterator :
	public IIterator<T>
{
public:
	NullIterator(std::unique_ptr<IIterator<U>> p_iter) :
		m_iter(std::move(p_iter))
	{ }

	void start() override { m_iter->start(); }
	bool valid() const override { return m_iter->valid(); }
	void next() override
	{
		m_iter->next();
	}
	T current() const override
	{
		DBSI_CHECK_PRECOND(valid());
		return m_my_T;
	}

private:
	T m_my_T;
	std::unique_ptr<IIterator<U>> m_iter;
};


/*
* This class contains the main database data structures
* such as the index, but also acts as a visitor to the
* std::variant returned by `parse_query`, leading to an
* elegant loop in the `main` function below.
*/
class QueryApplication
{
public:
	QueryApplication(bool log_plan_types) :
		m_done(false),
		m_log_plan_types(log_plan_types)
	{ }

	void operator()(const EmptyQuery&) {}

	void operator()(const BadQuery& e)
	{
		std::cerr << "Bad query. Error: " << e.error << std::endl;
	}

	void operator()(const QuitQuery&)
	{
		std::cout << "Exiting..." << std::endl;
		m_done = true;
	}

	void operator()(const LoadQuery& q)
	{
		const auto start_time = std::chrono::system_clock::now();

		std::ifstream file(q.filename, std::ios::binary);

		if (!file)
		{
			std::cerr << "Unfortunately the given file '"
				<< q.filename << "' cannot be opened." << std::endl;
			return;
		}

		// TODO: what does the file parser do if the file is corrupted?

		auto file_iter = autoencode(m_dict, create_turtle_file_parser(file));

		size_t add_count = 0;
		file_iter->start();
		while (file_iter->valid())
		{
			m_idx.add(file_iter->current());
			file_iter->next();
			++add_count;
		}

		const auto end_time = std::chrono::system_clock::now();

		std::cout << "Loaded " << add_count << " triples in " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
			<< "ms." << std::endl;
	}

	void operator()(const CountQuery& q)
	{
		SelectQuery q2;
		q2.match = q.match;
		(*this)(q2);
	}

	void operator()(const SelectQuery& q)
	{
		const bool print_mode = (!q.projection.empty());
		const auto start_time = std::chrono::system_clock::now();
		auto iter = evaluate_patterns(q.match);
		const auto planning_time = std::chrono::system_clock::now();

		// header
		if (print_mode)
		{
			std::cout << "----------" << std::endl;
			for (const auto& v : q.projection)
				std::cout << v.name << '\t';
			std::cout << std::endl;
		}

		size_t count = 0;
		iter->start();
		while (iter->valid())
		{
			// get current map
			const auto vm = iter->current();

			if (print_mode)
			{
				// print columns in this row
				for (size_t i = 0; i < q.projection.size(); ++i)
				{
					auto vm_iter = vm.find(q.projection[i]);
					if (vm_iter != vm.end())
					{
						std::cout << std::visit(DbsiToStringVisitor(),
							vm_iter->second) << '\t';
					}
					else
					{
						// in this case the user has mentioned a variable
						// in the projection which is not present in the
						// patterns
						std::cout << q.projection[i].name << '\t';
					}
				}
				std::cout << std::endl;
			}

			iter->next();
			++count;
		}

		// footer
		if (print_mode)
			std::cout << "----------" << std::endl;

		const auto end_time = std::chrono::system_clock::now();

		std::cout << count << " results obtained in " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
			<< "ms (= " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(planning_time - start_time).count()
			<< "ms planning + " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - planning_time).count()
			<< "ms evaluation)." << std::endl;
	}

	bool done() const
	{
		return m_done;
	}

private:
	std::unique_ptr<IVarMapIterator> evaluate_patterns(std::vector<TriplePattern> pats)
	{
		if (pats.empty())
		{
			// if there is an empty where clause, then all triples
			// satisfy the query, by vacuosity
			return std::make_unique<NullIterator<VarMap, CodedTriple>>(m_idx.full_scan());
		}
		else
		{
			// first need to encode the patterns
			std::vector<CodedTriplePattern> coded_pats;
			std::transform(pats.begin(), pats.end(), std::back_inserter(coded_pats),
				[this](const TriplePattern& pat) { return encode(m_dict, pat); });

			// join optimisation!
			joins::smart_join_order_opt(coded_pats);

			if (m_log_plan_types)
			{
				std::cout << "\t--> NLJ over patterns with types ";
				for (const auto& pat : coded_pats)
					std::cout << trip_pat_type_str(pattern_type(pat)) << ' ';
				std::cout << std::endl;
			}

			return autodecode(m_dict,
				joins::create_nested_loop_join_iterator(m_idx, std::move(coded_pats)));
		}
	}

private:
	bool m_done;
	const bool m_log_plan_types;
	Dictionary m_dict;
	RDFIndex m_idx;
};


void show_help()
{
	std::cout << "-h : Print help. If using this option, no other options can be used." << std::endl;
	std::cout << "-L : Show join plan selection types. Good for debugging performance "
		"issues. If used, it must be appear before any -i or -f options." << std::endl;
	std::cout << "-i query : Execute query/queries." << std::endl;
	std::cout << "-f filename : Execute query/queries from file." << std::endl;
	std::cout << "Using either -i or -f will open the application in non-interactive "
		"mode, and the application will exit automatically after running all "
		"given commands. Not using -i or -f will open the application in interactive "
		"mode, where you can type what you want, and have to manually close with `QUIT`." << std::endl;
}


int main(int argc, char* argv[])
{
	if (argc == 2 && std::string(argv[1]) == "-h")
	{
		show_help();
		return 0;
	}

	const bool log_plan_types = (argc > 1 && std::string(argv[1]) == "-L");
	const int cmd_start_idx = log_plan_types ? 2 : 1;
	const int num_commands = (argc - cmd_start_idx) / 2;

	QueryApplication app(log_plan_types);

	if (num_commands > 0)  // noninteractive mode
	{
		for (int i = 0; i < num_commands && !app.done(); ++i)
		{
			std::string cmd = argv[2 * i + cmd_start_idx];
			std::string arg = argv[2 * i + cmd_start_idx + 1];
			if (cmd == "-i")
			{
				std::stringstream cmd_in(arg);
				while (!app.done() && cmd_in)
				{
					std::visit(app, parse_query(cmd_in));
				}
			}
			else if (cmd == "-f")
			{
				std::ifstream cmd_in(arg, std::ios::binary);
				if (!cmd_in)
				{
					std::cerr << "Cannot open file '" << arg << "'." << std::endl;
					return 1;
				}
				while (!app.done() && cmd_in)
				{
					std::visit(app, parse_query(cmd_in));
				}
			}
			else
			{
				std::cerr << "Bad command '" << cmd
					<< "', must either be '-i' or '-f'. Showing help."
					<< std::endl;
				show_help();
				return 1;
			}
		}
	}
	else  // interactive mode
	{
		while (!app.done())
		{
			std::visit(app, parse_query(std::cin));
		}
	}

	return 0;
}
