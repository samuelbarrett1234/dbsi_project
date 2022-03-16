#include <iostream>
#include <fstream>
#include "dbsi_dictionary.h"
#include "dbsi_rdf_index.h"
#include "dbsi_turtle.h"
#include "dbsi_query.h"
#include "dbsi_dictionary_utils.h"


using namespace dbsi;


void load(Dictionary& dict, std::string filename)
{
	std::ifstream file(filename, std::ios::binary);
	// TODO: does the file exist?

	// TODO: what does the file parser do if the file is corrupted?

	auto file_iter = autoencode(dict, create_turtle_file_parser(file));

	file_iter->start();
	while (file_iter->valid())
	{
		std::cout << file_iter->current().sub << '\t'
			<< file_iter->current().pred << '\t'
			<< file_iter->current().obj << '\t'
			<< std::endl;
		file_iter->next();
	}

	// TODO: insert `file_iter` into `idx`
}


int main()
{
	Dictionary dict;
	RDFIndex idx;

	load(dict, "E:/dbsi/LUBM-001-mat.ttl");
}
