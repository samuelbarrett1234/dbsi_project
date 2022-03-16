#include "dbsi_pattern_utils.h"


namespace dbsi
{


bool pattern_matches(const Term& t, const Resource& r)
{
	struct ResourcePatternVisitor
	{
		bool operator()(const Resource& r2)
		{
			return r == r2;
		}
		bool operator()(const Variable&)
		{
			return true;
		}

		const Resource& r;
	};
	return std::visit(ResourcePatternVisitor{ r }, t);
}


bool pattern_matches(const CodedTerm& t, const CodedResource& r)
{
	struct CodedResourcePatternVisitor
	{
		bool operator()(const CodedResource& r2)
		{
			return r == r2;
		}
		bool operator()(const Variable&)
		{
			return true;
		}

		const CodedResource& r;
	};
	return std::visit(CodedResourcePatternVisitor{ r }, t);
}


bool pattern_matches(const TriplePattern& pat, const Triple& t)
{
	return pattern_matches(pat.sub, t.sub) &&
		pattern_matches(pat.pred, t.pred) &&
		pattern_matches(pat.obj, t.obj);
}


bool pattern_matches(const CodedTriplePattern& pat, const CodedTriple& t)
{
	return pattern_matches(pat.sub, t.sub) &&
		pattern_matches(pat.pred, t.pred) &&
		pattern_matches(pat.obj, t.obj);
}


}  // namespace dbsi
