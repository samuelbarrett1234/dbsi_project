#ifndef DBSI_RDF_INDEX_HELPER
#define DBSI_RDF_INDEX_HELPER


#include <vector>
#include <variant>
#include <unordered_map>
#include "dbsi_types.h"


/*
* The RDF data is stored as a table with six columns,
* following the work of this paper:
* Boris Motik, Yavor Nenov, Robert Piro, Ian Horrocks, and Dan Olteanu. Parallel Materialisation
* of Datalog Programs in Centralised, Main-Memory RDF Systems. Proc. of the 28th AAAI Conf.
* on Artificial Intelligence (AAAI 2014), pages 129–137, Quebec City, Quebec, Canada, July 27–
* 31 2014. AAAI Press.
* 
* The purpose of this file is to 'construct' the types used
* for indexing in the RDFIndex class. This is nontrivial because
* the types are circular: the indices hold iterators to the table,
* and the table holds iterators to the indices.
* 
* The ultimate takeaway from this file are the types `Table`,
* `SingleIndex` and `PairIndex`. They are as follows.
* 
* `Table` is a vector of `TripleRows`. A `TripleRow` has a coded
* triple, and three pointer-like objects.
* 
* Note: throughout this file, "table iterators" are a synonym for
* plain-old offsets. We cannot use std::vector iterators because
* they are invalidated on reallocation!
*
* The first of these, `n_p`, is an iterator pointing to another
* element in the table which has the same `pred` value, in such
* a way that all triples with the same predicate can be reached
* by starting from the 'correct' start point.
* The 'correct' start point for a given predicate is given precisely
* in the `SingleIndex` corresponding to `pred`.
* 
* The other two pointers, `n_sp` and `n_op`, are like `n_p` insofar as
* they represent all triples with a given `sub` (or `obj`, resp.)
* but they are grouped by `pred`.
* An important difference is that these pointers come in exactly
* one of two forms. A table iterator or a PairIndexIterator.
* 
* In the former case, the value of `pred` changes, so instead you must
* go *via* the PairIndexIterator to reach the corresponding table
* entry.
* Note: understanding this, it is clear that you never really need to
* treat these pointers as anything other than a table iterator. This
* is exactly right! So, to make this as convenient as possible, a
* visitor struct `TripleRowVisitor`.
* 
* It has been designed this way to make insertion of triples easy.
* It, crucially, has the effect that you can alter the start point
* of the pair index (if a new triple is inserted) without altering
* all of the table entries which point to that element!
*/


namespace dbsi
{
namespace rdf_idx_helper
{


template<typename ParentT>
struct TableHelper
{
	typedef size_t TableIterator;

	struct SingleIndexIterator :
		public ParentT::SingleIndexIterator
	{
		SingleIndexIterator() = default;
		SingleIndexIterator(typename ParentT::SingleIndexIterator i) :
			ParentT::SingleIndexIterator(i)
		{ }
	};
	struct PairIndexIterator :
		public ParentT::PairIndexIterator
	{
		PairIndexIterator() = default;
		PairIndexIterator(typename ParentT::PairIndexIterator i) :
			ParentT::PairIndexIterator(i)
		{ }
	};

	typedef std::variant<PairIndexIterator, TableIterator> IndexTableIterVariant;
	struct TripleRow
	{
		TripleRow(CodedTriple t, IndexTableIterVariant n_sp,
			IndexTableIterVariant n_op, TableIterator n_p) :
			t(std::move(t)), n_sp(std::move(n_sp)),
			n_op(std::move(n_op)), n_p(std::move(n_p))
		{ }
		CodedTriple t;
		IndexTableIterVariant n_sp, n_op;
		TableIterator n_p;
	};

	typedef std::vector<TripleRow> Table;
};


struct IndexHelper :
	public TableHelper<IndexHelper>
{
	struct SingleTermIndexEntry
	{
		TableIterator offset;  // pointer to head
		size_t size;  // total number of elements

		// invariant: size == 0 iff offset == end
	};

	/*
	* note: single index is unordered map here, rather than vector,
	* differing from the paper's implementation, because we want resizing
	* to preserve iterators.
	*/
	typedef std::unordered_map<CodedResource, SingleTermIndexEntry> SingleIndex;
	typedef std::unordered_map<std::pair<CodedResource, CodedResource>, TableIterator> PairIndex;

	struct SingleIndexIterator :
		public SingleIndex::const_iterator
	{
		SingleIndexIterator() = default;
		SingleIndexIterator(typename SingleIndex::const_iterator i) :
			SingleIndex::const_iterator(i)
		{ }
	};
	struct PairIndexIterator :
		public PairIndex::const_iterator
	{
		PairIndexIterator() = default;
		PairIndexIterator(typename PairIndex::const_iterator i) :
			PairIndex::const_iterator(i)
		{ }
	};
};


using Table = typename IndexHelper::Table;
using TableIterator = typename IndexHelper::TableIterator;
using SingleIndex = typename IndexHelper::SingleIndex;
using PairIndex = typename IndexHelper::PairIndex;
using TripleIndex = std::unordered_map<CodedTriple, TableIterator>;
using TripleRow = IndexHelper::TripleRow;
using IndexTableIterVariant = IndexHelper::IndexTableIterVariant;
using SingleTermIndexEntry = IndexHelper::SingleTermIndexEntry;


// representing a null offset / invalid table iterator
static const TableIterator TABLE_END = static_cast<TableIterator>(-1);


/*
* This struct has an explanation at the top of the file. But
* its usage will almost always be one of:
* std::visit(TripleRowVisitor(), my_triple_row.n_sp)
* std::visit(TripleRowVisitor(), my_triple_row.n_op)
*/
struct TripleRowVisitor
{
	TableIterator operator()(const PairIndex::const_iterator& iter) const
	{
		return iter->second;
	}
	TableIterator operator()(const TableIterator& iter) const
	{
		return iter;
	}
};


}  // namespace rdf_idx_helper
}  // namespace dbsi


#endif  // DBSI_RDF_INDEX_HELPER
