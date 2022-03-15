#ifndef DBSI_TYPES_H
#define DBSI_TYPES_H


#include <string>
#include <variant>


namespace dbsi
{


struct Literal { std::string val; };
struct IRI { std::string val; };
struct Variable { std::string name; };
typedef std::variant<Literal, IRI> Resource;
typedef std::variant<Variable, Resource> Term;
struct Triple
{
	Term sub, pred, obj;
};


}  // namespace dbsi


#endif  // DBSI_TYPES_H
