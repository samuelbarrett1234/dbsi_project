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
			* Note: here I am requiring `cur_score > best_score`, rather
			* than `cur_score < best_score` as is given in the exam question
			* paper, because my nested loop join implementation does things
			* the other way around. Experimentation with < versus > confirms
			* my suspicions, because as is, it is MUCH faster.
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
* `adj_mat` : a vector of length N*N (for some N) such that element
*             i + N * j equals true iff there is an edge from i->j
*             where 0 <= i,j < N.
* returns   : a matrix whose element i + N * j equals the length of
*	          the shortest path from i to j, measured in number of
*             edges, or static_cast<size_t>(-1) if there is no such
*             path.
* 
* This is an instance of the Floyd-Warshall algorithm. Since it
* is relatively standard, I will not explain it fully.
*/
std::vector<size_t> all_pairs_shortest_paths(const size_t N, std::vector<bool> adj_mat)
{
	DBSI_CHECK_PRECOND(N * N == adj_mat.size());
	static const size_t bad_value = static_cast<size_t>(-1);

	// dist[i + N * j] gives the shortest distance from i to j
	// at the given iteration. -1 indicates invalid value.
	std::vector<size_t> dist(adj_mat.size(), bad_value);

	for (size_t i = 0; i * i < N * N; ++i)
		dist[i + i * N] = 0;  // distance from i to i is 0
	for (size_t k = 0; k < adj_mat.size(); ++k)
		if (adj_mat[k])
			dist[k] = 1;  // edge distances

	// main Floyd-Warshall loop
	for (size_t k = 0; k < N; ++k)
		for (size_t i = 0; i < N; ++i)
			for (size_t j = 0; j < N; ++j)
				if (dist[i + N * j] > dist[i + N * k] + dist[k + N * j])
					dist[i + N * j] = dist[i + N * k] + dist[k + N * j];

	return dist;
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
	return true;
}


/*
* Build the adjacency matrix for the patterns in the range
* [begin, end) according to the criteria that:
* T_i -> T_j iff
* T_i and T_j do not have disjoint variable sets,
* and the variable set of T_j is NOT a proper
* subset of the variable set of T_i.
*
* The rationale for this is explained in the accompanying
* report, but in short, this produces a graph whose
* most `central' vertices are
*/
std::vector<bool> build_adj_mat(
	const std::vector<CodedTriplePattern>::const_iterator begin,
	const std::vector<CodedTriplePattern>::const_iterator end)
{
	// firstly, convert each coded triple pattern into
	// a coded variable map (but not caring about which
	// values are mapped to)
	std::vector<CodedVarMap> maps;
	std::transform(begin, end, std::back_inserter(maps), [](CodedTriplePattern pat)
		{ return extract_map(pat); });

	// now we're ready to compute the adjacencies
	const size_t N = maps.size();
	std::vector<bool> output(N * N, false);
	for (size_t i = 0; i < N; ++i)
	{
		for (size_t j = 0; j < N; ++j)
		{
			output[i + N * j] = (
				var_maps_disjoint(maps[i], maps[j])
				&& !var_map_key_subset(maps[j], maps[i])
				);
		}
	}
	return output;
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

		// firstly, try aggressively promote any patterns of SPO-type
		// (while computing the scores; if we don't find any of good
		// score then this will be a useful array later)
		std::vector<int> scores(N, -1);
		for (size_t i = cur_idx; i < N; ++i)
		{
			scores[i] = score_pattern(pattern_type(conditioned_patterns[i]));
			if (scores[i] == best_possible_score)
			{
				// found one!
				candidate_idx = i;
				break;
			}
		}

		// if that fails, pick a `central' pattern to promote.
		if (candidate_idx == static_cast<size_t>(-1))
		{
			// the notion of centrality is determined by the average
			// path length to other nodes, with ties broken by their
			// type-score

			const size_t N_remaining = N - cur_idx;
			const auto shortest_paths = all_pairs_shortest_paths(N_remaining,
				build_adj_mat(conditioned_patterns.begin() + cur_idx, conditioned_patterns.end()));
			DBSI_CHECK_POSTCOND(shortest_paths.size() == N_remaining * N_remaining);

			// <node, score> pairs
			std::vector<std::pair<size_t, size_t>> centrality_scores(
				N_remaining, std::make_pair(0, 0));
			for (size_t k = 0; k < shortest_paths.size(); ++k)
			{
				const size_t j = k / N_remaining, i = k - j * N_remaining;
				DBSI_CHECK_INVARIANT(k == i + j * N_remaining);
				centrality_scores[i].first = i;

				// we need to be careful about overflow here, because (size_t)(-1)
				// denotes not-a-path!
				if (shortest_paths[k] == static_cast<size_t>(-1)
					|| centrality_scores[i].second == static_cast<size_t>(-1))
					centrality_scores[i].second = static_cast<size_t>(-1);
				else
					centrality_scores[i].second += shortest_paths[k];
			}

			// sort and pick best vertices according to centrality
			// score first, then tie-breaking by SPO-score if necessary
			std::sort(centrality_scores.begin(), centrality_scores.end(),
				[&scores](auto& left, auto& right)
				{
					return (left.second < right.second
						&& left.second != static_cast<size_t>(-1)
						&& right.second != static_cast<size_t>(-1)) ||
						(left.second == right.second &&
							scores[left.first] < scores[right.first]);
				});

			// now we have our candidate!
			candidate_idx = centrality_scores[0].first;
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
