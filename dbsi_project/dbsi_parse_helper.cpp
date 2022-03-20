#include "dbsi_parse_helper.h"
#include <vector>


namespace dbsi
{


bool next_nonws_char(char& out_c, std::istream& in)
{
	while (std::isspace(out_c = in.get()));
	return (out_c != -1  /* EOF*/);
}


std::optional<Resource> parse_resource(std::istream& in)
{
	// this buffer is especially for reducing allocations between
	// resource parses.
	// the only downside is it is persistent, so technically, extremely
	// long IRIs/Literals will be held twice in memory, but that is
	// unlikely and it is a big improvement to load times.
	static std::vector<char> str(32);

	// the first character tells us whether this is
	// a literal/IRI
	char start_char;
	if (!next_nonws_char(start_char, in))
		return std::nullopt;

	if (start_char != '<' && start_char != '"')
		return std::nullopt;

	str.clear();
	const char end_char = (start_char == '<') ? '>' : '"';
	char next_char;

	while (in.good() && (next_char = in.get()) != end_char)
	{
		str.push_back(next_char);
	}

	// if the stream ended in the middle of an expression
	// (note: warnings about `next_char` being uninitialised
	// here are unfounded, because the while loop above is
	// guaranteed to perform at least one iteration.)
	if (end_char != next_char)
		return std::nullopt;

	if (start_char == '<')
		return IRI{ std::string(str.begin(), str.end()) };
	else
		return Literal{ std::string(str.begin(), str.end()) };
}


std::optional<Term> parse_term(std::istream& in)
{
	// this loop is similar to `next_nonws_char`, except
	// that it doesn't read the last char
	char start_char;
	while (std::isspace(start_char = in.peek()))
		in.get();  // advance by 1

	if (start_char == -1 /* EOF */)
		return std::nullopt;

	if (start_char == '?')
	{
		std::string var_name;
		in >> var_name;
		return Variable{ var_name };
	}
	else
		return parse_resource(in);
}


}  // namespace dbsi
