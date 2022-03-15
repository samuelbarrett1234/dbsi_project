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
	std::ifstream file(filename);
	// TODO: does the file exist?

	// TODO: what does the file parser do if the file is corrupted?

	auto file_iter = autoencode(dict, create_turtle_file_parser(file));

	// TODO: insert `file_iter` into `idx`
}


int main()
{
	Dictionary dict;
	RDFIndex idx;

	load(dict, "test.txt");
}
