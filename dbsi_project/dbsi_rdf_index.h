#ifndef DBSI_RDF_INDEX_H
#define DBSI_RDF_INDEX_H


#include <optional>
#include <memory>
#include "dbsi_types.h"
#include "dbsi_iterator.h"
#include "dbsi_rdf_index_helper.h"


namespace dbsi
{


/*
* This holds entire database, in encoded format, possibly
* using indexing.
*/
class RDFIndex
{
private:
	enum class IndexType
	{
		NONE,  // start from the start
		SPO,  // use SPO index to find start
		SP,  // use SP index to find start
		OP,  // use OP index to find start
		SUB,  // use sub index to find start
		PRED,  // use pred index to find start
		OBJ  // use obj index to find start
	};

	enum class EvaluationType
	{
		ALL,  // evaluate all rows
		NONE,  // evaluate no row except the one you start at
		SP,  // evaluate rows by following the sub-pred pointers
		P,  // evaluate rows by following the pred pointers
		OP  // evaluate rows by following the obj-pred pointers
	};

	// iterator class for producing evaluation results
	class IndexIterator :
		public ICodedVarMapIterator
	{
	public:
		IndexIterator(
			const rdf_idx_helper::Table& triples,
			CodedTriplePattern pattern,
			rdf_idx_helper::TableIterator start_idx,
			EvaluationType eval_type);

		void start() override;
		CodedVarMap current() const override;
		void next() override;
		bool valid() const override;

	private:
		void increment_idx();
		void inc_till_pattern_match();

	private:
		const rdf_idx_helper::Table& m_triples;
		const EvaluationType m_eval_type;
		const CodedTriplePattern m_pattern;
		const rdf_idx_helper::TableIterator m_start_idx;
		rdf_idx_helper::TableIterator m_cur_idx;

		/*
		* invariant: m_cur_map = bind(m_pattern, m_triples[m_cur_idx])
		* if valid(), else m_cur_map is std::nullopt if !valid()
		*/
		std::optional<CodedVarMap> m_cur_map;
	};

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
	*/
	std::unique_ptr<ICodedVarMapIterator> evaluate(CodedTriplePattern pattern) const;

private:
	/*
	* This function takes a pattern, and chooses (i) which, if any,
	* index to use to find the first element, and (ii) which, if any,
	* linked list structure to follow to evaluate the triples.
	* If the first element of the returned pair is none, start from
	* the start. Otherwise use the index on the relevant term type.
	*/
	std::pair<RDFIndex::IndexType, EvaluationType> plan_pattern(CodedTriplePattern pattern) const;

private:
	rdf_idx_helper::Table m_triples;
	rdf_idx_helper::SingleIndex m_sub_index, m_pred_index, m_obj_index;
	rdf_idx_helper::PairIndex m_sp_index, m_op_index;
	rdf_idx_helper::TripleIndex m_triple_index;
};


}  // namespace dbsi


#endif  // DBSI_RDF_INDEX_H
