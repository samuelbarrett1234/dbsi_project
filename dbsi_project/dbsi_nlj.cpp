#include "dbsi_nlj.h"
#include "dbsi_assert.h"
#include "dbsi_rdf_index.h"
#include "dbsi_pattern_utils.h"
#include <iostream>


namespace dbsi
{
namespace joins
{


class NestedLoopJoinIterator :
	public ICodedVarMapIterator
{
public:
	NestedLoopJoinIterator(
		const RDFIndex& rdf_idx,
		std::vector<CodedTriplePattern> patterns) :
		m_idx(rdf_idx),
		m_patterns(std::move(patterns))
	{ }

	void start() override
	{
		// clear any old iterators
		m_iter_depth.clear();
		// initialise with outermost loop
		m_iter_depth.push_back(m_idx.evaluate(m_patterns[0]));
		// start first iterator
		m_iter_depth[0]->start();
		// create remaining iterators
		update_iterators();
	}

	CodedVarMap current() const override
	{
		// note: even though it is an invariant that the size of
		// m_iter_depth is either 0 or equal to the size of m_patterns,
		// this function is explicitly required to relax the latter
		// case, because it is useful to build partial variable maps.
		DBSI_CHECK_PRECOND(valid());

		CodedVarMap vm;
		for (const auto& iter : m_iter_depth)
		{
			bool success = merge(vm, iter->current());

			// if this fails then the RDF index iterator
			// implementation is faulty, it should automatically
			// apply selections.
			DBSI_CHECK_POSTCOND(success);
		}

		return vm;
	}

	void next() override
	{
		DBSI_CHECK_PRECOND(valid());

		m_iter_depth.back()->next();
		update_iterators();
	}

	bool valid() const override
	{
		return !m_iter_depth.empty();
	}

private:
	void update_iterators()
	{
		// get rid of any finished iterators
		while (!m_iter_depth.empty() && !m_iter_depth.back()->valid())
		{
			m_iter_depth.pop_back();
			if (!m_iter_depth.empty())
				m_iter_depth.back()->next();
		}

		// bring the list back up to full strength
		while (!m_iter_depth.empty() && m_iter_depth.size() < m_patterns.size())
		{
			// get current pattern
			CodedTriplePattern pat = m_patterns[m_iter_depth.size()];
			// fill in any variables set by outer loops
			pat = substitute(current(), std::move(pat));
			// create iterator for next loop depth
			m_iter_depth.push_back(
				m_idx.evaluate(std::move(pat))
			);
			// start iterator
			m_iter_depth.back()->start();
		}
	}

private:
	const RDFIndex& m_idx;
	const std::vector<CodedTriplePattern> m_patterns;
	
	// this is a stack-like data structure of iterators, one corresponding
	// to each part of the join.
	// invariant: all iterators here are valid.
	// invariant: this vector is empty iff this iterator is invalid
	// invariant: if this iterator is nonempty then its size is equal
	// to that of `m_patterns`.
	std::vector<std::unique_ptr<ICodedVarMapIterator>> m_iter_depth;
};


std::unique_ptr<ICodedVarMapIterator> joins::create_nested_loop_join_iterator(
	const RDFIndex& rdf_idx, std::vector<CodedTriplePattern> patterns)
{
	return std::make_unique<NestedLoopJoinIterator>(rdf_idx, std::move(patterns));
}


}  // namespace joins
}  // namespace dbsi
