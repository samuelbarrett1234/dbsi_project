#ifndef DBSI_QUERY_H
#define DBSI_QUERY_H


#include <vector>
#include <string>
#include <variant>
#include <istream>
#include "dbsi_types.h"


namespace dbsi
{


struct BadQuery
{
	BadQuery(std::string e) :
		error(std::move(e))
	{ }
	std::string error;
};


struct SelectQuery
{
	std::vector<Variable> projection;
	std::vector<TriplePattern> match;
};


struct CountQuery
{
	std::vector<TriplePattern> match;
};


struct LoadQuery
{
	std::string filename;
};


struct QuitQuery {};


/*
* Read a single query from the given input string, which
* may contain zero, one, or multiple queries. In the case
* of any error (or lack of a query), `BadQuery` is returned.
* In all other cases, the foremost query in the string is
* read and returned.
*/
std::variant<BadQuery, SelectQuery, CountQuery, LoadQuery, QuitQuery> parse_query(std::istream& in);


}  // namespace dbsi


#endif  // DBSI_QUERY_H
