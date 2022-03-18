#include <algorithm>
#include <iterator>
#include "dbsi_nlj.h"
#include "dbsi_assert.h"
#include "dbsi_rdf_index.h"
#include "dbsi_pattern_utils.h"


namespace dbsi
{
namespace joins
{


/*
* Score a pattern based on how selective it is estimated
* to be.
* The ordering here is given by the ordering from the paper.
* Lower scores are more selective.
*/
constexpr int score_pattern(TriplePatternType type)
{
	switch (type)
	{
	case TriplePatternType::SPO:
		return 0;
	case TriplePatternType::SVO:
		return 1;
	case TriplePatternType::VPO:
		return 2;
	case TriplePatternType::SPV:
		return 3;
	case TriplePatternType::VVO:
		return 4;
	case TriplePatternType::SVV:
		return 5;
	case TriplePatternType::VPV:
		return 6;
	case TriplePatternType::VVV:
		return 7;
	}
}


/*
* Returns true iff the two coded var maps
* share no variables.
*/
bool var_maps_disjoint(const CodedVarMap& cvm1, const CodedVarMap& cvm2)
{
	auto iter1 = cvm1.cbegin();
	auto iter2 = cvm2.cbegin();
	while (iter1 != cvm1.cend() && iter2 != cvm2.cend())
	{
		if (iter1->first < iter2->first)
			++iter1;
		else if (iter1->first > iter2->first)
			++iter2;
		else
			return false;
	}
	return true;
}


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


void greedy_join_order_opt(std::vector<CodedTriplePattern>& patterns)
{
	static const size_t bad_idx = static_cast<size_t>(-1);

	CodedVarMap cvm;
	for (size_t cur_idx = 0; cur_idx < patterns.size(); ++cur_idx)
	{
		// firstly, get the pattern in range [cur_idx, patterns.size())
		// with lowest score, where the score is determined by the
		// pattern conditional on the maps from patterns in range
		// [0, cur_idx).
		size_t best_idx = bad_idx;
		int best_score = 10;  // or any number > 7
		CodedVarMap best_cvm;
		for (size_t i = cur_idx; i < patterns.size(); ++i)
		{
			// get the pattern's variable map, and its score after substitution
			CodedVarMap cur_cvm = extract_map(patterns[i]);
			const int cur_score = score_pattern(pattern_type(
				substitute(cvm, patterns[i])));

			/*
			* In addition to selecting only when the score is better,
			* we also want to only select when it's not going to create
			* a full cross product.
			* In order to avoid a full cross product, they either have
			* to share a variable in common, or the new CVM has no variables
			* (in which case it is just an index lookup).
			* 
			* Note: in the special case where there are no unavoidable
			* cross products, we have to arbitrarily pick a next pattern
			* to be joined. WLOG we may pick the one at `cur_idx`. This
			* case is handled by the `else if`.
			*/
			if (cur_score < best_score &&
				(cur_cvm.empty() || !var_maps_disjoint(cvm, cur_cvm)))
			{
				best_idx = i;
				best_score = cur_score;
				best_cvm = std::move(cur_cvm);
			}
			else if (best_idx == bad_idx)
			{
				DBSI_CHECK_INVARIANT(i == cur_idx);
				best_idx = i;
				best_cvm = std::move(cur_cvm);
			}
		}

		DBSI_CHECK_INVARIANT(best_idx < patterns.size() && best_idx >= cur_idx);

		// now that we have selected a pattern, move it to index `cur_idx`,
		// then add its variables to `cvm`
		std::swap(patterns[cur_idx], patterns[best_idx]);

		// merge CVMs
		const bool ok = merge(cvm, best_cvm);
		DBSI_CHECK_INVARIANT(ok);
	}
}


/*
* Returns true iff all keys in the lhs appear in the rhs.
*/
bool var_map_key_subset(const CodedVarMap& lhs, const CodedVarMap& rhs)
{
	auto iter1 = lhs.begin();
	auto iter2 = rhs.begin();
	while (iter1 != lhs.end() && iter2 != rhs.end())
	{
		if (iter1->first < iter2->first)
			return false;
		else if (iter1->first > iter2->first)
			++iter2;
		else
		{
			++iter1;
			++iter2;
		}
	}
	return (iter1 == lhs.end());
}


void smart_join_order_opt(std::vector<CodedTriplePattern>& patterns)
{
	// invariant: `patterns` and `conditioned_patterns` will
	// maintain the same order, and the ith element of the latter
	// will equal the ith element of the former after substituting
	// all variables mentioned in the patterns in the range [0, cur_idx)
	std::vector<CodedTriplePattern> conditioned_patterns = patterns;
	const size_t N = patterns.size();
	constexpr int best_possible_score = score_pattern(TriplePatternType::SPO);

	for (size_t cur_idx = 0; cur_idx < N; ++cur_idx)
	{
		size_t candidate_idx = static_cast<size_t>(-1);
		const size_t N_remaining = N - cur_idx;

		// firstly, try aggressively promote any patterns of SPO-type
		// (while computing the scores; if we don't find any of good
		// score then this will be a useful array later)
		std::vector<int> scores(N, -1);
		for (size_t i = cur_idx; i < N; ++i)
		{
			const auto pat_type = pattern_type(conditioned_patterns[i]);
			scores[i] = score_pattern(pat_type);
			if (pat_type == TriplePatternType::SPO)
			{
				// found one!
				candidate_idx = i;
				break;
			}
		}

		// if that fails, pick a different pattern to promote.
		if (candidate_idx == static_cast<size_t>(-1))
		{
			// <node, score> pairs
			std::vector<std::pair<size_t, size_t>> spo_scores(
				N_remaining, std::make_pair(0, 0));
			for (size_t i = 0; i < N_remaining; ++i)
			{
				spo_scores[i].first = i + cur_idx;
				for (size_t j = 0; j < N_remaining; ++j)
				{
					if (i != j &&
						var_map_key_subset(extract_map(conditioned_patterns[cur_idx + j]),
						extract_map(conditioned_patterns[cur_idx + i])))
						++spo_scores[i].second;
				}
			}

			// sort and pick best vertices according to centrality
			// score first, then tie-breaking by SPO-score if necessary
			std::sort(spo_scores.begin(), spo_scores.end(),
				[&scores](auto& left, auto& right)
				{
					DBSI_CHECK_INVARIANT(scores[left.first] >= 0);
					DBSI_CHECK_INVARIANT(scores[right.first] >= 0);
					return (left.second > right.second) ||
						(left.second == right.second &&
							scores[left.first] < scores[right.first]);
				});

			// now we have our candidate!
			candidate_idx = spo_scores[0].first;
		}

		// swap elements so our chosen one is at `cur_idx`
		std::swap(patterns[cur_idx], patterns[candidate_idx]);
		std::swap(conditioned_patterns[cur_idx], conditioned_patterns[candidate_idx]);
		// now get th variables this triple pattern sets
		const auto updating_map = extract_map(conditioned_patterns[cur_idx]);
		// make the substitutions in the remaining triple patterns
		for (size_t update_idx = cur_idx; update_idx < N; ++update_idx)
		{
			conditioned_patterns[update_idx] = substitute(
				updating_map, conditioned_patterns[update_idx]);
		}
	}
}


}  // namespace joins
}  // namespace dbsi
