#ifndef DBSI_RDF_INDEX_H
#define DBSI_RDF_INDEX_H


#include <memory>
#include <vector>
#include <optional>
#include "dbsi_types.h"
#include "dbsi_iterator.h"


namespace dbsi
{


/*
* This holds entire database, in encoded format, possibly
* using indexing.
*/
class RDFIndex
{
public:
	/*
	* Add a coded triple to the database.
	* WARNING: invalidates any currently-alive iterators.
	* Please do not insert while you read!
	*/
	void add(CodedTriple t);

	/*
	* Create an iterator to begin evaluation over
	* a certain pattern (the returned triples will
	* satisfy this).
	* You can optionally request that these triples
	* be given in a certain order. The index is not
	* required to satisfy this if you ask for it,
	* and in general it will only be done where it can
	* be done efficiently.
	* You can check the sortedness of the triple
	* iterator once it has been created.
	*/
	std::unique_ptr<ICodedVarMapIterator> evaluate(CodedTriplePattern pattern,
		std::optional<TripleOrder> request_output_order = std::nullopt) const;

private:
	std::vector<CodedTriple> m_triples;
};


}  // namespace dbsi


#endif  // DBSI_RDF_INDEX_H
