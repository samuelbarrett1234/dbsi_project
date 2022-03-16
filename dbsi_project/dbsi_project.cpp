#include <iostream>
#include <fstream>
#include <chrono>
#include "dbsi_dictionary.h"
#include "dbsi_rdf_index.h"
#include "dbsi_turtle.h"
#include "dbsi_query.h"
#include "dbsi_dictionary_utils.h"


using namespace dbsi;


void load(Dictionary& dict, RDFIndex& idx, std::string filename)
{
	std::ifstream file(filename, std::ios::binary);
	// TODO: does the file exist?

	// TODO: what does the file parser do if the file is corrupted?

	auto file_iter = autoencode(dict, create_turtle_file_parser(file));

	file_iter->start();
	while (file_iter->valid())
	{
		idx.add(file_iter->current());
		file_iter->next();
	}

	// TODO: insert `file_iter` into `idx`
}


void select_test(Dictionary& dict, RDFIndex& idx)
{
	CodedTriplePattern pat{
		Variable{"x"}, dict.encode(IRI{"<http://swat.cse.lehigh.edu/onto/univ-bench.owl#member>"}), Variable{"y"}
	};
	size_t count = 0;
	auto iter = autodecode(dict, idx.evaluate(pat));
	iter->start();
	while (iter->valid())
	{
		++count;
		iter->next();
	}
	std::cout << "Total count: " << count << std::endl;
}


int main()
{
	Dictionary dict;
	RDFIndex idx;

	std::cout << "Loading..." << std::endl;
	auto start = std::chrono::system_clock::now();

	load(dict, idx, "E:/dbsi/LUBM-001-mat.ttl");

	auto between = std::chrono::system_clock::now();

	std::cout << "Done. Time: " <<
		std::chrono::duration_cast<std::chrono::milliseconds>(between - start).count()
		<< "ms. Starting query..." << std::endl;

	select_test(dict, idx);

	auto done = std::chrono::system_clock::now();

	std::cout << "Done. Time: " <<
		std::chrono::duration_cast<std::chrono::milliseconds>(done - between).count()
		<< "ms." << std::endl;

	std::cin.get();

	return 0;
}
