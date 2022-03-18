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


enum class TriplePatternType
{
	VVV, VVO, VPV, SVV, VPO, SVO, SPV, SPO
};


/*
* Get a string reprsentation of a triple plan type.
*/
std::string trip_pat_type_str(TriplePatternType type);


template<typename ResT>
TriplePatternType pattern_type(const GeneralTriplePattern<ResT>& pattern)
{
	if (std::holds_alternative<Variable>(pattern.sub))  // if subject variable
	{
		if (std::holds_alternative<Variable>(pattern.pred))  // if predicate variable
		{
			if (std::holds_alternative<Variable>(pattern.obj))  // if object variable
			{
				return TriplePatternType::VVV;
			}
			else  // if object known
			{
				return TriplePatternType::VVO;
			}
		}
		else  // if predicate known
		{
			if (std::holds_alternative<Variable>(pattern.obj))  // if object variable
			{
				return TriplePatternType::VPV;
			}
			else  // if object known
			{
				return TriplePatternType::VPO;
			}
		}
	}
	else  // if subject known
	{
		if (std::holds_alternative<Variable>(pattern.pred))  // if predicate variable
		{
			if (std::holds_alternative<Variable>(pattern.obj))  // if object variable
			{
				return TriplePatternType::SVV;
			}
			else  // if object known
			{
				return TriplePatternType::SVO;
			}
		}
		else  // if predicate known
		{
			if (std::holds_alternative<Variable>(pattern.obj))  // if object variable
			{
				return TriplePatternType::SPV;
			}
			else  // if object known
			{
				return TriplePatternType::SPO;
			}
		}
	}
}


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


template<>
struct hash<dbsi::GeneralTriple<dbsi::CodedResource>>
{
	std::size_t operator()(const dbsi::GeneralTriple<dbsi::CodedResource>& t) const
	{
		// calling `dbsi::my_hash` here helps in the case where ResT
		// is just an integer, in which case the hash is the identity
		// function, causing collisions
		return ((t.sub << (2 * sizeof(dbsi::CodedResource) / 3))
			^ (t.pred << (sizeof(dbsi::CodedResource) / 3)) ^ t.obj);
	}
};


template <typename ResT>
struct hash<dbsi::GeneralTriple<ResT>>
{
	std::size_t operator()(const dbsi::GeneralTriple<ResT>& t) const
	{
		return (dbsi::my_hash(std::hash<ResT>()(t.sub)) * 37
			+ dbsi::my_hash(std::hash<ResT>()(t.pred))) * 37
			+ dbsi::my_hash(std::hash<ResT>()(t.obj));
	}
};


template <typename ResT>
struct hash<dbsi::GeneralTriplePattern<ResT>>
{
	std::size_t operator()(const dbsi::GeneralTriplePattern<ResT>& t) const
	{
		return (std::hash<GeneralTerm<ResT>>()(t.sub) * 37
			+ std::hash<GeneralTerm<ResT>>()(t.pred)) * 37
			+ std::hash<GeneralTerm<ResT>>()(t.obj);
	}
};


template <typename T1, typename T2>
struct hash<pair<T1, T2>>
{
	std::size_t operator()(const pair<T1, T2>& p) const
	{
		return std::hash<T1>()(p.first)
			+ 37 * std::hash<T2>()(p.second);
	}
};

}  // namespace std


#endif  // DBSI_TYPES_H
