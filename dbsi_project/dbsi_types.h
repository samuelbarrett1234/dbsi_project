#ifndef DBSI_TYPES_H
#define DBSI_TYPES_H


#include <string>
#include <variant>
#include <map>


namespace dbsi
{


struct Literal
{
	inline bool operator == (const Literal& other) const
	{
		return val == other.val;
	}
	std::string val;
};


struct IRI
{
	inline bool operator == (const IRI& other) const
	{
		return val == other.val;
	}
	std::string val;
};


struct Variable
{
	inline bool operator == (const Variable& other) const
	{
		return name == other.name;
	}
	std::string name;
};

typedef std::variant<Literal, IRI> Resource;
typedef size_t CodedResource;
typedef std::variant<Variable, Resource> Term;
typedef std::variant<Variable, CodedResource> CodedTerm;
typedef std::map<Variable, Resource> VarMap;


struct Triple
{
	Resource sub, pred, obj;
};


struct TriplePattern
{
	Term sub, pred, obj;
};


struct CodedTriple
{
	CodedResource sub, pred, obj;
};


struct CodedTriplePattern
{
	CodedTerm sub, pred, obj;
};


enum class TripleOrder
{
	SPO, SOP, OSP, OPS, PSO, POS
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


}  // namespace std


#endif  // DBSI_TYPES_H
