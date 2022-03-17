#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include "dbsi_dictionary.h"
#include "dbsi_rdf_index.h"
#include "dbsi_turtle.h"
#include "dbsi_query.h"
#include "dbsi_dictionary_utils.h"
#include "dbsi_nlj.h"


using namespace dbsi;


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

	void operator()(const BadQuery&)
	{
		std::cerr << "Bad, or no, query." << std::endl;
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
			std::chrono::duration_cast<std::chrono::milliseconds>(start_time - end_time).count()
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
			std::chrono::duration_cast<std::chrono::milliseconds>(start_time - end_time).count()
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
				std::cout << std::visit(DbsiToStringVisitor(),
					vm.at(q.projection[i])) << '\t';
			}
			std::cout << std::endl;

			iter->next();
			++count;
		}
		std::cout << "----------" << std::endl;

		const auto end_time = std::chrono::system_clock::now();

		std::cout << count << " results obtained in " <<
			std::chrono::duration_cast<std::chrono::milliseconds>(start_time - end_time).count()
			<< "ms." << std::endl;
	}

	bool done() const
	{
		return m_done;
	}

private:
	std::unique_ptr<IVarMapIterator> evaluate_patterns(std::vector<TriplePattern> pats)
	{
		// first need to encode the patterns
		std::vector<CodedTriplePattern> coded_pats;
		std::transform(pats.begin(), pats.end(), std::back_inserter(coded_pats),
			[this](const TriplePattern& pat) { return encode(m_dict, pat); });

		return autodecode(m_dict,
			joins::create_nested_loop_join_iterator(m_idx, std::move(coded_pats)));
	}

private:
	bool m_done;
	Dictionary m_dict;
	RDFIndex m_idx;
};


int main()
{
	QueryApplication app;

	while (!app.done())
	{
		std::visit(app, parse_query(std::cin));
	}

	return 0;
}
