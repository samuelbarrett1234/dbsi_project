#ifndef DBSI_PATTERN_UTILS_H
#define DBSI_PATTERN_UTILS_H


#include "dbsi_types.h"
#include <optional>


namespace dbsi
{


/*
* These functions return true iff there exist variable solutions
* in the left argument to make it equal to the right argument.
*/
template<typename ResT>
bool pattern_matches(const GeneralTerm<ResT>& t, const ResT& r)
{
	struct PatternVisitor
	{
		bool operator()(const ResT& r2)
		{
			return r == r2;
		}
		bool operator()(const Variable&)
		{
			return true;
		}

		const ResT& r;
	};
	return std::visit(PatternVisitor{ r }, t);
}
template<typename ResT>
bool pattern_matches(const GeneralTriplePattern<ResT>& pat, const GeneralTriple<ResT>& t)
{
	// unfortunately this isn't so simple because we need to check
	// that the variable assignments are consistent
	// (although this could probably be optimised)
	return (bool)bind(pat, t);
}


/*
* Try to merge the `in` map into the `out` map.
* Returns true on success, false if the two are inconsistent (i.e.
* they assign different values).
* WARNING: if this function returns false then the result of `out`
* is invalid, we do not guarantee to restore it to its state
* prior to this call.
*/
template<typename VarMapT>
bool merge(VarMapT& out, const VarMapT& in)
{
	// variation on a merge-sort, with error checking
	// and automatic insertion from `in` to `out`
	auto iter_out = out.begin();
	auto iter_in = in.cbegin();
	while (iter_out != out.end() && iter_in != in.cend())
	{
		if (iter_out->first < iter_in->first)
			++iter_out;
		else if (iter_out->first > iter_in->first)  // `in` value not found in `out`
		{
			// insert into `out` (note that this will not invalidate
			// any iterators)
			out[iter_in->first] = iter_in->second;
			++iter_in;
		}
		else
		{
			if (iter_out->second != iter_in->second)
				return false;  // inconsistent
			// else advance both
			++iter_out;
			++iter_in;
		}
	}
	// there may be some more elements for us to copy over
	while (iter_in != in.cend())
	{
		// the hint `out.end()` means this should be O(1)
		// complexity
		out.insert(out.end(), *iter_in);
		++iter_in;
	}

	return true;
}


/*
* Perform a variable substitution.
*/
template<typename ResT>
GeneralTerm<ResT> substitute(const GeneralVarMap<ResT>& vm, GeneralTerm<ResT> t)
{
	struct TermSubber
	{
		GeneralTerm<ResT> operator()(ResT r)
		{
			return r;
		}
		GeneralTerm<ResT> operator()(Variable v)
		{
			auto iter = vm.find(v);
			if (iter == vm.end())
				return v;
			else
				return iter->second;
		}
		const GeneralVarMap<ResT>& vm;
	};
	return std::visit(TermSubber{ vm }, std::move(t));
}
template<typename ResT>
GeneralTriplePattern<ResT> substitute(const GeneralVarMap<ResT>& vm, GeneralTriplePattern<ResT> pat)
{
	pat.sub = substitute(vm, pat.sub);
	pat.pred = substitute(vm, pat.pred);
	pat.obj = substitute(vm, pat.obj);
	return pat;
}


/*
* These functions attempt to extract a variable mapping
* which makes the former match the latter, or returns nothing
* on failure. This is a generalisation of the series of
* `pattern_matches` functions.
*/
template<typename ResT>
std::optional<GeneralVarMap<ResT>> bind(GeneralTerm<ResT> t, ResT r)
{
	struct BindVisitor
	{
		std::optional<GeneralVarMap<ResT>> operator()(const ResT& r2)
		{
			if (r == r2)
				return GeneralVarMap<ResT>();  // no variables required
			else
				return std::nullopt;  // failed match
		}
		std::optional<GeneralVarMap<ResT>> operator()(const Variable& v)
		{
			GeneralVarMap<ResT> vm;
			vm[v] = r;
			return vm;
		}

		ResT r;
	};
	return std::visit(BindVisitor{ std::move(r) }, t);	
}
template<typename ResT>
std::optional<GeneralVarMap<ResT>> bind(GeneralTriplePattern<ResT> pat, GeneralTriple<ResT> t)
{
	// not the most elegant way of doing it, but this function is called a lot
	// so it is important to be efficient!

	GeneralVarMap<ResT> result;

	if (std::holds_alternative<Variable>(pat.sub))
	{
		// this will never fail
		result[std::get<Variable>(pat.sub)] = t.sub;
	}
	else if (std::get<ResT>(pat.sub) != t.sub)
		return std::nullopt;
	if (std::holds_alternative<Variable>(pat.pred))
	{
		auto [iter, inserted] = result.insert(std::make_pair(std::get<Variable>(pat.pred), t.pred));
		if (!inserted && iter->second != t.pred)
			return std::nullopt;
	}
	else if (std::get<ResT>(pat.pred) != t.pred)
		return std::nullopt;
	if (std::holds_alternative<Variable>(pat.obj))
	{
		auto [iter, inserted] = result.insert(std::make_pair(std::get<Variable>(pat.obj), t.obj));
		if (!inserted && iter->second != t.obj)
			return std::nullopt;
	}
	else if (std::get<ResT>(pat.obj) != t.obj)
		return std::nullopt;

	return result;
}


/*
* Create a variable map from the keys present in the given
* term or triple pattern, and map them to arbitrary values.
*/
template<typename ResT>
GeneralVarMap<ResT> extract_map(GeneralTerm<ResT> t)
{
	struct ExtractionVisitor
	{
		GeneralVarMap<ResT> operator()(ResT r)
		{
			return GeneralVarMap<ResT>();
		}
		GeneralVarMap<ResT> operator()(Variable v)
		{
			GeneralVarMap<ResT> vm;
			vm[v] = ResT();  // map to arbitrary value
			return vm;
		}
	};
	return std::visit(ExtractionVisitor(), std::move(t));
}
template<typename ResT>
GeneralVarMap<ResT> extract_map(GeneralTriplePattern<ResT> pat)
{
	auto vm1 = extract_map(pat.sub),
		vm2 = extract_map(pat.pred),
		vm3 = extract_map(pat.obj);

	// merge them, we don't care about the values
	vm1.merge(std::move(vm2));
	vm1.merge(std::move(vm3));

	return vm1;
}


}  // namespace dbsi


#endif  // DBSI_PATTERN_UTILS_H
