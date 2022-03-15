#ifndef DBSI_TYPES_H
#define DBSI_TYPES_H


#include <string>
#include <variant>
#include <map>


namespace dbsi
{


struct Literal { std::string val; };
struct IRI { std::string val; };
struct Variable { std::string name; };

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


#endif  // DBSI_TYPES_H
