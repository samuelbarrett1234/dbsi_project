#include <sstream>
#include "dbsi_query.h"
#include "dbsi_parse_helper.h"


namespace dbsi
{


std::variant<BadQuery, SelectQuery, CountQuery, LoadQuery, QuitQuery> parse_query(std::istream& in)
{
	if (!in)
		return BadQuery("Empty input.");

	std::string first_word;
	in >> first_word;

	if (first_word == "QUIT")
		return QuitQuery();

	// skip any whitespace after the command,
	// because we expect there to be more
	in >> std::ws;

	if (first_word == "LOAD")
	{
		LoadQuery lq;
		std::getline(in, lq.filename);
		return lq;
	}

	if (first_word != "SELECT" && first_word != "COUNT")
		return BadQuery("Invalid command: " + first_word + ", must be QUIT/LOAD/SELECT/COUNT.");

	// read in the arguments that come before the WHERE clause
	std::vector<Variable> args;
	std::string next_word;
	in >> next_word;
	while (next_word != "WHERE" && (bool)in)
	{
		if (next_word[0] != '?')
			return BadQuery("Variables must start with question marks, but yours is " + next_word);

		args.push_back(Variable{ std::move(next_word) });
		in >> next_word;
	}

	// read bracket
	in >> next_word;
	if (next_word != "{")
		return BadQuery("Missing bracket after WHERE.");

	std::optional<Term> maybe_term;
	std::vector<TriplePattern> pattern;
	while (next_word != "}")
	{
		TriplePattern t;

		maybe_term = parse_term(in);
		if (!maybe_term)
		{
			std::stringstream errmsg;
			errmsg << "Bad subject for term at index " << pattern.size()
				<< " in where clause.";
			return BadQuery(errmsg.str());
		}
		t.sub = *maybe_term;

		maybe_term = parse_term(in);
		if (!maybe_term)
		{
			std::stringstream errmsg;
			errmsg << "Bad predicate for term at index " << pattern.size()
				<< " in where clause.";
			return BadQuery(errmsg.str());
		}
		t.pred = *maybe_term;

		maybe_term = parse_term(in);
		if (!maybe_term)
		{
			std::stringstream errmsg;
			errmsg << "Bad object for term at index " << pattern.size()
				<< " in where clause.";
			return BadQuery(errmsg.str());
		}
		t.obj = *maybe_term;

		pattern.push_back(std::move(t));

		in >> next_word;

		if (next_word != "}" && next_word != ".")
			return BadQuery("Bad where-clause triple-pattern delimiter: " + next_word);
	}

	if (first_word == "SELECT")
		return SelectQuery{ std::move(args), std::move(pattern) };
	else
		return CountQuery{ std::move(pattern) };
}


}  // namespace dbsi
