#ifndef DBSI_PARSE_HELPER_H
#define DBSI_PARSE_HELPER_H


#include <istream>
#include <optional>
#include "dbsi_types.h"


namespace dbsi
{


/*
* Skips all current whitespace in the stream `in`,
* and outputs the first non-whitespace character
* to `out_c` if it exists (and returns true), or
* returns false if EOF happened earlier. In either
* case, `out_c` has the possibility of being modified.
*/
bool next_nonws_char(char& out_c, std::istream& in);


/*
* Try to parse a resource from the given input stream.
* If bad syntax, return std::nullopt.
* A sufficient condition for syntax to be bad is that:
* the first non-whitespace character is neither < nor "
* It does not necessarily consume whitespace afterwards,
* but will consume whitespace beforehand.
*/
std::optional<Resource> parse_resource(std::istream& in);


/*
* Try to parse a term from the given input stream.
* If bad syntax, return std::nullopt.
* A sufficient condition for syntax to be bad is that:
* the first non-whitespace character is neither < nor " nor ?
* It does not necessarily consume whitespace afterwards,
* but will consume whitespace beforehand.
*/
std::optional<Term> parse_term(std::istream& in);


}  // namespace dbsi


#endif  // DBSI_PARSE_HELPER_H
