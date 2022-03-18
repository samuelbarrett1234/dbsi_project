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
	QueryApplication() :
		m_done(false)
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
		const auto start_time = std::chrono::system_clock::now();
		auto iter = evaluate_patterns(q.match);

		iter->start();
		size_t count = 0;
		while (iter->valid())
		{
			++count;
			iter->next();
		}

		const auto end_time = std::chrono::system_clock::now();

		std::cout << "Total count: " << count << ", obtained in " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
			<< "ms." << std::endl;
	}

	void operator()(const SelectQuery& q)
	{
		const auto start_time = std::chrono::system_clock::now();
		auto iter = evaluate_patterns(q.match);

		// header
		std::cout << "----------" << std::endl;
		for (const auto& v : q.projection)
			std::cout << v.name << '\t';
		std::cout << std::endl;

		size_t count = 0;
		iter->start();
		while (iter->valid())
		{
			// get current map
			const auto vm = iter->current();

			// print columns
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

			iter->next();
			++count;
		}
		std::cout << "----------" << std::endl;

		const auto end_time = std::chrono::system_clock::now();

		std::cout << count << " results obtained in " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()
			<< "ms." << std::endl;
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

			return autodecode(m_dict,
				joins::create_nested_loop_join_iterator(m_idx, std::move(coded_pats)));
		}
	}

private:
	bool m_done;
	Dictionary m_dict;
	RDFIndex m_idx;
};


int main(int argc, char* argv[])
{
	if (argc == 2)
	{
		if (std::string(argv[1]) != "-h")
			std::cerr << "Invalid option " << argv[1]
			<< ", showing help:" << std::endl;

		std::cout << "-h : print help." << std::endl;
		std::cout << "-i query : execute query/queries." << std::endl;
		std::cout << "-f filename : execute query/queries from file." << std::endl;
		return 0;
	}

	if (argc % 2 == 0)
	{
		std::cerr << "Invalid number of command line arguments given." << std::endl;
		return 1;
	}
	const int num_commands = (argc - 1) / 2;

	QueryApplication app;

	// start by parsing any command line arguments
	for (int i = 0; i < num_commands && !app.done(); ++i)
	{
		std::string cmd = argv[2 * i + 1];
		std::string arg = argv[2 * i + 2];
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
				<< "', must either be '-i' or '-f'."
				<< std::endl;
			return 1;
		}
	}

	while (!app.done())
	{
		std::visit(app, parse_query(std::cin));
	}

	return 0;
}
