#ifndef DBSI_QUERY_H
#define DBSI_QUERY_H


#include <vector>
#include <string>
#include <variant>
#include "dbsi_types.h"


namespace dbsi
{


struct BadQuery {};


struct SelectQuery
{
	std::vector<Variable> projection;
	std::vector<TriplePattern> match;
};


struct CountQuery
{
	std::vector<TriplePattern> match;
};


/*
* Parse a query and return a struct representing
* the detected type of query, and its relevant info.
*/
std::variant<BadQuery, SelectQuery, CountQuery> parse_query(std::string q);


}  // namespace dbsi


#endif  // DBSI_QUERY_H
