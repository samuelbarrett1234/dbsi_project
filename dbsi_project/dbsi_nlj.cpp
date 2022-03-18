#include "dbsi_nlj.h"
#include "dbsi_assert.h"
#include "dbsi_rdf_index.h"
#include "dbsi_pattern_utils.h"


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
	{
		DBSI_CHECK_PRECOND(m_patterns.size() > 0);
	}

	void start() override
	{
		// clear any old iterators
		m_iter_depth.clear();

		if (!m_patterns.empty())
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
			// even with the remark above, it is still necessary
			// for all iterators to be valid!
			DBSI_CHECK_INVARIANT(iter->valid());

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
		// while there exists a finished iterator at the back,
		// or we need to create more iterators
		while (!m_iter_depth.empty() && (
			!m_iter_depth.back()->valid() || m_iter_depth.size() < m_patterns.size()))
		{
			// clear all finished iterators
			while (!m_iter_depth.empty() && !m_iter_depth.back()->valid())
			{
				m_iter_depth.pop_back();
				if (!m_iter_depth.empty())
				{
					DBSI_CHECK_INVARIANT(m_iter_depth.back()->valid());
					m_iter_depth.back()->next();
				}
			}

			// bring the list back up to full strength
			// (more than one will be added where necessary
			// due to the outer while loop; we need to be careful
			// that we don't do it here, in case newly created
			// iterators are invalid.)
			if (!m_iter_depth.empty() && m_iter_depth.size() < m_patterns.size())
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

				// NOTE: no guarantee that this new iterator is valid,
				// this will be checked again in the outer while loop.
			}
		}

		DBSI_CHECK_INVARIANT(m_iter_depth.empty() ||
			m_iter_depth.size() == m_patterns.size());
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


std::unique_ptr<ICodedVarMapIterator> create_nested_loop_join_iterator(
	const RDFIndex& rdf_idx, std::vector<CodedTriplePattern> patterns)
{
	DBSI_CHECK_PRECOND(patterns.size() > 0);
	return std::make_unique<NestedLoopJoinIterator>(rdf_idx, std::move(patterns));
}


}  // namespace joins
}  // namespace dbsi
