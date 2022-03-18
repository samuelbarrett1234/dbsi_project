#ifndef DBSI_TYPES_H
#define DBSI_TYPES_H


#include <string>
#include <variant>
#include <map>


namespace dbsi
{


struct Literal
{
	std::string val;

	inline bool operator == (const Literal& other) const { return val == other.val; }
	inline bool operator != (const Literal& other) const { return val != other.val; }
	inline bool operator < (const Literal& other) const { return val < other.val; }
	inline bool operator <= (const Literal& other) const { return val <= other.val; }
	inline bool operator > (const Literal& other) const { return val > other.val; }
	inline bool operator >= (const Literal& other) const { return val >= other.val; }
};


struct IRI
{
	std::string val;

	inline bool operator == (const IRI& other) const { return val == other.val; }
	inline bool operator != (const IRI& other) const { return val != other.val; }
	inline bool operator < (const IRI& other) const { return val < other.val; }
	inline bool operator <= (const IRI& other) const { return val <= other.val; }
	inline bool operator > (const IRI& other) const { return val > other.val; }
	inline bool operator >= (const IRI& other) const { return val >= other.val; }
};


struct Variable
{
	std::string name;

	inline bool operator == (const Variable& other) const { return name == other.name; }
	inline bool operator != (const Variable& other) const { return name != other.name; }
	inline bool operator < (const Variable& other) const { return name < other.name; }
	inline bool operator <= (const Variable& other) const { return name <= other.name; }
	inline bool operator > (const Variable& other) const { return name > other.name; }
	inline bool operator >= (const Variable& other) const { return name >= other.name; }
};


typedef std::variant<Literal, IRI> Resource;
typedef size_t CodedResource;


template<typename ResT>
using GeneralTerm = std::variant<Variable, ResT>;
template<typename ResT>
using GeneralVarMap = std::map<Variable, ResT>;


typedef GeneralTerm<Resource> Term;
typedef GeneralTerm<CodedResource> CodedTerm;
typedef GeneralVarMap<Resource> VarMap;
typedef GeneralVarMap<CodedResource> CodedVarMap;


template<typename ResT>
struct GeneralTriple
{
	ResT sub, pred, obj;

	inline bool operator == (const GeneralTriple<ResT>& other) const
	{
		return sub == other.sub && pred == other.pred && obj == other.obj;
	}
	inline bool operator != (const Literal& other) const { return !(*this == other); }
};


template<typename ResT>
struct GeneralTriplePattern
{
	GeneralTerm<ResT> sub, pred, obj;

	inline bool operator == (const GeneralTriple<ResT>& other) const
	{
		return sub == other.sub && pred == other.pred && obj == other.obj;
	}
	inline bool operator != (const Literal& other) const { return !(*this == other); }
};


typedef GeneralTriple<Resource> Triple;
typedef GeneralTriple<CodedResource> CodedTriple;
typedef GeneralTriplePattern<Resource> TriplePattern;
typedef GeneralTriplePattern<CodedResource> CodedTriplePattern;


enum class TripleOrder
{
	SPO, SOP, OSP, OPS, PSO, POS
};


// works on Resources or Terms, via calling or via std::visit
struct DbsiToStringVisitor
{
	std::string operator()(const Literal& l)
	{
		return '"' + l.val + '"';
	}
	std::string operator()(const IRI& l)
	{
		return '<' + l.val + '>';
	}
	std::string operator()(const Resource& r)
	{
		return std::visit(*this, r);
	}
	std::string operator()(const Variable& v)
	{
		return v.name;
	}
	std::string operator()(const Term& t)
	{
		return std::visit(*this, t);
	}
};


}  // namespace dbsi


/*
* implementation of standard hash functions of the above structs,
* so they can be used as keys of hash maps etc
*/
namespace std
{


template <>
struct hash<dbsi::Literal>
{
	std::size_t operator()(const dbsi::Literal& k) const
	{
		return std::hash<std::string>()(k.val);
	}
};


template <>
struct hash<dbsi::IRI>
{
	std::size_t operator()(const dbsi::IRI& k) const
	{
		return std::hash<std::string>()(k.val) * 3;
	}
};


template <>
struct hash<dbsi::Variable>
{
	std::size_t operator()(const dbsi::Variable& k) const
	{
		return std::hash<std::string>()(k.name) * 5;
	}
};


template <typename ResT>
struct hash<dbsi::GeneralTriple<ResT>>
{
	std::size_t operator()(const dbsi::GeneralTriple<ResT>& t) const
	{
		return std::hash<ResT>()(t.sub)
			+ 7 * std::hash<ResT>()(t.pred)
			+ 11 * std::hash<ResT>()(t.obj);
	}
};


template <typename ResT>
struct hash<dbsi::GeneralTriplePattern<ResT>>
{
	std::size_t operator()(const dbsi::GeneralTriplePattern<ResT>& t) const
	{
		return std::hash<ResT>()(t.sub)
			+ 13 * std::hash<ResT>()(t.pred)
			+ 17 * std::hash<ResT>()(t.obj);
	}
};


template <typename T1, typename T2>
struct hash<pair<T1, T2>>
{
	std::size_t operator()(const pair<T1, T2>& p) const
	{
		return std::hash<T1>()(p.first)
			+ 19 * std::hash<T2>()(p.second);
	}
};

}  // namespace std


#endif  // DBSI_TYPES_H
