#include "dbsi_rdf_index.h"
#include "dbsi_pattern_utils.h"
#include "dbsi_assert.h"
#ifdef DBSI_CHECKING_INVARIANTS
#include <unordered_set>  // used for checking integrity in debug mode
#endif


namespace dbsi
{


void RDFIndex::add(CodedTriple t)
{
	// don't insert duplicates!
	if (m_triple_index.find(t) != m_triple_index.end())
		return;

	using rdf_idx_helper::TABLE_END;

	// fill in defaults where applicable
	m_sub_index.insert(std::make_pair(t.sub, rdf_idx_helper::SingleTermIndexEntry{ TABLE_END, 0 }));
	m_pred_index.insert(std::make_pair(t.pred, rdf_idx_helper::SingleTermIndexEntry{ TABLE_END, 0 }));
	m_obj_index.insert(std::make_pair(t.obj, rdf_idx_helper::SingleTermIndexEntry{ TABLE_END, 0 }));

	const auto new_offset = m_triples.size();

	auto sp_iter = m_sp_index.find(std::make_pair(t.sub, t.pred));
	auto op_iter = m_op_index.find(std::make_pair(t.obj, t.pred));

	/* Important note:
	* Below, where we use `m_sub_index[t.sub].offset` and the
	* corresponding version for `obj`, what does this mean?
	* If instead we'd put the natural `cend()` iterator, this
	* would make the `n_sp` and `n_op` pointers only be linked
	* lists *within* each sp/op group.
	* However, we want these pointers to contain different `p`s,
	* grouped by `p`.
	* To achieve this, observe that if an s/p combination does
	* not already exist, then `m_sub_index[t.sub]` will contain
	* a different `pred` to the current one.
	* Since we always update `m_sub_index[t.sub]` to be the new
	* value, the quantity `m_sub_index[t.sub].pred` will, over
	* the course of the RDFIndex's lifetime, never repeat a
	* `pred`.
	* But: what happens if another entry corresponding to the
	* old `pred` gets inserted? It is updated in the index, and
	* thus there is no need to update it in the table!
	*/

	DBSI_CHECK_INVARIANT(m_sub_index[t.sub].offset == TABLE_END  // first occurrence of this `sub`
		|| sp_iter != m_sp_index.cend()  // this `sub/pred` combo has appeared before
		|| m_triples[m_sub_index[t.sub].offset].t.pred != t.pred);  // else, has a new `pred`
	// similar check for `obj`:
	DBSI_CHECK_INVARIANT(m_obj_index[t.obj].offset == TABLE_END
		|| op_iter != m_op_index.cend()
		|| m_triples[m_obj_index[t.obj].offset].t.pred != t.pred);

	// decide which n_sp and n_op pointers to use
	rdf_idx_helper::IndexTableIterVariant sp_source, op_source;
	if (sp_iter == m_sp_index.cend())
	{
		if (m_sub_index[t.sub].offset == rdf_idx_helper::TABLE_END)
		{
			// the first time this subject is entered into the DB,
			// there is nothing for n_sp to point to!
			sp_source = rdf_idx_helper::TABLE_END;
		}
		else
		{
			// pick an arbitrary (sub, pred') pair for some other pred' != pred
			const CodedTriple& other_trip = m_triples[m_sub_index[t.sub].offset].t;
			sp_source = (rdf_idx_helper::IndexHelper::PairIndexIterator)m_sp_index.find(
				std::make_pair(other_trip.sub, other_trip.pred));
		}
	}
	else
		sp_source = sp_iter->second;

	if (op_iter == m_op_index.cend())
	{
		if (m_obj_index[t.obj].offset == rdf_idx_helper::TABLE_END)
		{
			// the first time this subject is entered into the DB,
			// there is nothing for n_sp to point to!
			op_source = rdf_idx_helper::TABLE_END;
		}
		else
		{
			// pick an arbitrary (obj, pred') pair for some other pred' != pred
			const CodedTriple& other_trip = m_triples[m_obj_index[t.obj].offset].t;
			op_source = (rdf_idx_helper::IndexHelper::PairIndexIterator)m_op_index.find(
				std::make_pair(other_trip.obj, other_trip.pred));
		}
	}
	else
		op_source = op_iter->second;

	// add to table
	m_triples.emplace_back(
		t, sp_source, op_source,
		m_pred_index[t.pred].offset
	);

	/* Now, we don't *always* want to set this new triple to
	* be the 'front' of the sub/obj index. We ONLY want to do
	* this when the triple introduces a new value of `pred`,
	* or if the triple agrees with the value of `pred`
	* currently being pointed to. It is all about making sure
	* we update the offset iff the new triple has a `pred`
	* from the group at the front of the linked list.
	*/
	if (sp_iter == m_sp_index.cend() ||
		m_triples[m_sub_index[t.sub].offset].t.pred == t.pred)
		m_sub_index[t.sub].offset = new_offset;
	if (op_iter == m_op_index.cend() ||
		m_triples[m_obj_index[t.obj].offset].t.pred == t.pred)
		m_obj_index[t.obj].offset = new_offset;

	// always add to the `pred` index as this case
	// is slightly simpler
	m_pred_index[t.pred].offset = new_offset;

	// always increment sizes in the single term indices
	++m_sub_index[t.sub].size;
	++m_pred_index[t.pred].size;
	++m_obj_index[t.obj].size;

	m_sp_index[std::make_pair(t.sub, t.pred)] = new_offset;
	m_op_index[std::make_pair(t.obj, t.pred)] = new_offset;
	m_triple_index[t] = new_offset;

#ifdef DBSI_CHECKING_INVARIANTS
	// in debug mode, check integrity every 100 triples added.
	// this check is expensive so don't do it every time.
	if (m_triples.size() % 5000 == 0)
		check_integrity();
#endif  // DBSI_CHECKING_INVARIANTS
}


std::unique_ptr<ICodedVarMapIterator> RDFIndex::evaluate(CodedTriplePattern pattern) const
{
	auto [index_type, eval_type] = plan_pattern(pattern);
	auto start_index = rdf_idx_helper::TABLE_END;

	switch (index_type)
	{
	case IndexType::NONE:
		// in almost all cases, this means the start index is 0,
		// however in the special case where the DB is empty, 0
		// is an unnacceptable index (see the assertion at the
		// bottom of this function).
		start_index = (m_triples.empty()) ? rdf_idx_helper::TABLE_END : 0;
		break;
	case IndexType::SUB:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.sub));
		{
			auto iter = m_sub_index.find(std::get<CodedResource>(pattern.sub));
			if (iter != m_sub_index.end())
				start_index = iter->second.offset;
			// else no triple exists
		}
		break;
	case IndexType::PRED:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.pred));
		{
			auto iter = m_pred_index.find(std::get<CodedResource>(pattern.pred));
			if (iter != m_pred_index.end())
				start_index = iter->second.offset;
			// else no triple exists
		}
		break;
	case IndexType::OBJ:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.obj));
		{
			auto iter = m_obj_index.find(std::get<CodedResource>(pattern.obj));
			if (iter != m_obj_index.end())
				start_index = iter->second.offset;
			// else no triple exists
		}
		break;
	case IndexType::SP:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.sub));
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.pred));
		{
			auto iter = m_sp_index.find(std::make_pair(
				std::get<CodedResource>(pattern.sub),
				std::get<CodedResource>(pattern.pred)));
			if (iter != m_sp_index.end())
				start_index = iter->second;
			// else no triple exists
		}
		break;
	case IndexType::OP:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.obj));
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.pred));
		{
			auto iter = m_op_index.find(std::make_pair(
				std::get<CodedResource>(pattern.obj),
				std::get<CodedResource>(pattern.pred)));
			if (iter != m_op_index.end())
				start_index = iter->second;
			// else no triple exists
		}
		break;
	case IndexType::SPO:
		// if this fails, `plan_pattern` is faulty
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.sub));
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.pred));
		DBSI_CHECK_POSTCOND(std::holds_alternative<CodedResource>(pattern.obj));
		{
			auto iter = m_triple_index.find({
				std::get<CodedResource>(pattern.sub),
				std::get<CodedResource>(pattern.pred),
				std::get<CodedResource>(pattern.obj)
				});
			if (iter != m_triple_index.end())
				start_index = iter->second;
			// else no triple exists
		}
		break;
	}

	DBSI_CHECK_INVARIANT(start_index < m_triples.size() || start_index == rdf_idx_helper::TABLE_END);

	return std::make_unique<IndexIterator>(m_triples, pattern, start_index, eval_type);
}


std::unique_ptr<ICodedTripleIterator> RDFIndex::full_scan() const
{
	// keeping this class internal to this function because it's so simple
	class FullRDFScanIterator :
		public ICodedTripleIterator
	{
	public:
		FullRDFScanIterator(const rdf_idx_helper::Table& table) :
			m_table(table), m_idx(0)
		{ }

		void start() override { m_idx = 0; }

		bool valid() const override { return m_idx < m_table.size(); }

		void next() override
		{
			DBSI_CHECK_PRECOND(valid());
			++m_idx;
		}

		CodedTriple current() const override
		{
			DBSI_CHECK_PRECOND(valid());
			return m_table[m_idx].t;
		}

	private:
		const rdf_idx_helper::Table& m_table;
		size_t m_idx;
	};

	return std::make_unique<FullRDFScanIterator>(m_triples);
}


std::pair<RDFIndex::IndexType, RDFIndex::EvaluationType> RDFIndex::plan_pattern(
	CodedTriplePattern pattern) const
{
	RDFIndex::IndexType index_type;
	RDFIndex::EvaluationType eval_type;

	/*
	* Pick index and evaluation method based on the pattern type
	*/
	switch (pattern_type(pattern))
	{
	case TriplePatternType::VVV:
		index_type = RDFIndex::IndexType::NONE;
		eval_type = RDFIndex::EvaluationType::ALL;
		break;

	case TriplePatternType::VVO:
		index_type = RDFIndex::IndexType::OBJ;
		eval_type = RDFIndex::EvaluationType::OP;
		break;

	case TriplePatternType::VPV:
		index_type = RDFIndex::IndexType::PRED;
		eval_type = RDFIndex::EvaluationType::P;
		break;

	case TriplePatternType::VPO:
		index_type = RDFIndex::IndexType::OP;
		eval_type = RDFIndex::EvaluationType::OP;
		break;

	case TriplePatternType::SVV:
		index_type = RDFIndex::IndexType::SUB;
		eval_type = RDFIndex::EvaluationType::SP;
		break;

	case TriplePatternType::SVO:
	{
		// have to make a decision between subject and object
		// based on index selectivity (see paper)
		auto sub_iter = m_sub_index.find(std::get<CodedResource>(pattern.sub));
		auto obj_iter = m_obj_index.find(std::get<CodedResource>(pattern.obj));
		if (sub_iter == m_sub_index.end() || obj_iter == m_obj_index.end())
		{
			// however obviously if either sub or obj do not
			// exist then the query will return empty results.
			// the cheapest way to guarantee this is the following:
			index_type = RDFIndex::IndexType::NONE;
			eval_type = RDFIndex::EvaluationType::NONE;
		}
		else if (sub_iter->second.size < obj_iter->second.size)  //	`sub` more selective
		{
			index_type = RDFIndex::IndexType::SUB;
			eval_type = RDFIndex::EvaluationType::SP;
		}
		else  // `obj` more selective
		{
			index_type = RDFIndex::IndexType::OBJ;
			eval_type = RDFIndex::EvaluationType::OP;
		}
	}
	break;

	case TriplePatternType::SPV:
		index_type = RDFIndex::IndexType::SP;
		eval_type = RDFIndex::EvaluationType::SP;
		break;

	case TriplePatternType::SPO:
		index_type = RDFIndex::IndexType::SPO;
		eval_type = RDFIndex::EvaluationType::NONE;
		break;

	default:
		DBSI_CHECK_POSTCOND(false);  // ???
	}

	return std::make_pair(index_type, eval_type);
}


#ifdef DBSI_CHECKING_INVARIANTS


void RDFIndex::check_integrity()
{
	using rdf_idx_helper::TABLE_END;
	rdf_idx_helper::TripleRowVisitor get_row;

	// check the triple index
	DBSI_CHECK_INVARIANT(m_triples.size() == m_triple_index.size());
	for (const auto& kv : m_triple_index)
		DBSI_CHECK_INVARIANT(m_triples[kv.second].t == kv.first);

	// check the linked list pointers
	for (size_t i = 0; i < m_triples.size(); ++i)
	{
		const size_t n_sp = std::visit(get_row, m_triples[i].n_sp),
			n_op = std::visit(get_row, m_triples[i].n_op);

		// check the linked list pointers have the guarantees
		// of the same sub/pred/obj respectively
		DBSI_CHECK_INVARIANT(n_sp == TABLE_END ||
			m_triples[n_sp].t.sub == m_triples[i].t.sub);
		DBSI_CHECK_INVARIANT(m_triples[i].n_p == TABLE_END ||
			m_triples[m_triples[i].n_p].t.pred == m_triples[i].t.pred);
		DBSI_CHECK_INVARIANT(n_op == TABLE_END ||
			m_triples[n_op].t.obj == m_triples[i].t.obj);
	}

	// check that the pair indices point to the first of a linked
	// list which (i) terminates, and (ii) holds only the correct
	// values, and (iii) contains ALL of the right values
	for (const auto& kv : m_sp_index)
	{
		const auto [sub, pred] = kv.first;

		std::unordered_set<size_t> idxs_found;

		// `tab_idx` is the linked list pointer we are following.
		// `i` is only here to ensure termination, and isn't strictly
		// necessary.
		size_t tab_idx = kv.second;
		idxs_found.insert(tab_idx);
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.sub == sub);
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.pred == pred);

			if (!std::holds_alternative<size_t>(m_triples[tab_idx].n_sp))
				break;  // changing to a different `pred` group
			else
				tab_idx = std::get<size_t>(m_triples[tab_idx].n_sp);

			if (tab_idx == TABLE_END)
				break;

			// check we've not found a linked-list loop!
			DBSI_CHECK_INVARIANT(idxs_found.find(tab_idx) == idxs_found.end());
			idxs_found.insert(tab_idx);
		}

		// now do a full table scan to make sure we got the precise set
		// of matching triples
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			if (m_triples[i].t.sub == sub && m_triples[i].t.pred == pred)
				DBSI_CHECK_INVARIANT(idxs_found.find(i) != idxs_found.end());
		}
	}
	// same as for above but for OP rather than SP
	for (const auto& kv : m_op_index)
	{
		const auto [obj, pred] = kv.first;

		std::unordered_set<size_t> idxs_found;

		// `tab_idx` is the linked list pointer we are following.
		// `i` is only here to ensure termination, and isn't strictly
		// necessary.
		size_t tab_idx = kv.second;
		idxs_found.insert(tab_idx);
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.obj == obj);
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.pred == pred);

			if (!std::holds_alternative<size_t>(m_triples[tab_idx].n_op))
				break;  // changing to a different `pred` group
			else
				tab_idx = std::get<size_t>(m_triples[tab_idx].n_op);

			if (tab_idx == TABLE_END)
				break;

			// check we've not found a linked-list loop!
			DBSI_CHECK_INVARIANT(idxs_found.find(tab_idx) == idxs_found.end());
			idxs_found.insert(tab_idx);
		}

		// now do a full table scan to make sure we got the precise set
		// of matching triples
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			if (m_triples[i].t.obj == obj && m_triples[i].t.pred == pred)
				DBSI_CHECK_INVARIANT(idxs_found.find(i) != idxs_found.end());
		}
	}

	// check that the single indices point to the first of a linked
	// list which (i) terminates, and (ii) holds only the correct
	// values, and (iii) contains ALL of the right values
	for (const auto& kv : m_sub_index)
	{
		std::unordered_set<size_t> idxs_found;

		// `tab_idx` is the linked list pointer we are following.
		// `i` is only here to ensure termination, and isn't strictly
		// necessary.
		size_t tab_idx = kv.second.offset;
		idxs_found.insert(tab_idx);
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.sub == kv.first);

			tab_idx = std::visit(get_row, m_triples[tab_idx].n_sp);

			if (tab_idx == TABLE_END)
				break;

			// check we've not found a linked-list loop!
			DBSI_CHECK_INVARIANT(idxs_found.find(tab_idx) == idxs_found.end());
			idxs_found.insert(tab_idx);
		}

		// now do a full table scan to make sure we got the precise set
		// of matching triples
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			if (m_triples[i].t.sub == kv.first)
				DBSI_CHECK_INVARIANT(idxs_found.find(i) != idxs_found.end());
		}

		DBSI_CHECK_INVARIANT(idxs_found.size() == kv.second.size);
	}
	// same for `obj`
	for (const auto& kv : m_obj_index)
	{
		std::unordered_set<size_t> idxs_found;

		// `tab_idx` is the linked list pointer we are following.
		// `i` is only here to ensure termination, and isn't strictly
		// necessary.
		size_t tab_idx = kv.second.offset;
		idxs_found.insert(tab_idx);
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.obj == kv.first);

			tab_idx = std::visit(get_row, m_triples[tab_idx].n_op);

			if (tab_idx == TABLE_END)
				break;

			// check we've not found a linked-list loop!
			DBSI_CHECK_INVARIANT(idxs_found.find(tab_idx) == idxs_found.end());
			idxs_found.insert(tab_idx);
		}

		// now do a full table scan to make sure we got the precise set
		// of matching triples
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			if (m_triples[i].t.obj == kv.first)
				DBSI_CHECK_INVARIANT(idxs_found.find(i) != idxs_found.end());
		}

		DBSI_CHECK_INVARIANT(idxs_found.size() == kv.second.size);
	}
	// and finally, something similar but slightly simpler for `pred`
	for (const auto& kv : m_pred_index)
	{
		std::unordered_set<size_t> idxs_found;

		// `tab_idx` is the linked list pointer we are following.
		// `i` is only here to ensure termination, and isn't strictly
		// necessary.
		size_t tab_idx = kv.second.offset;
		idxs_found.insert(tab_idx);
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			DBSI_CHECK_INVARIANT(m_triples[tab_idx].t.pred == kv.first);

			tab_idx = m_triples[tab_idx].n_p;

			if (tab_idx == TABLE_END)
				break;

			// check we've not found a linked-list loop!
			DBSI_CHECK_INVARIANT(idxs_found.find(tab_idx) == idxs_found.end());
			idxs_found.insert(tab_idx);
		}

		DBSI_CHECK_INVARIANT(idxs_found.size() == kv.second.size);

		// now do a full table scan to make sure we got the precise set
		// of matching triples
		for (size_t i = 0; i < m_triples.size(); ++i)
		{
			if (m_triples[i].t.pred == kv.first)
				DBSI_CHECK_INVARIANT(idxs_found.find(i) != idxs_found.end());
		}
	}
}


#else


void RDFIndex::check_integrity() { /* no-op */ }


#endif  // DBSI_CHECKING_INVARIANTS


RDFIndex::IndexIterator::IndexIterator(
	const rdf_idx_helper::Table& triples,
	CodedTriplePattern pattern,
	rdf_idx_helper::TableIterator start_idx, EvaluationType eval_type) :
	m_eval_type(eval_type), m_triples(triples),
	m_start_idx(start_idx), m_cur_idx(triples.size()),
	m_pattern(std::move(pattern))
{
	DBSI_CHECK_PRECOND(m_start_idx < m_triples.size() || m_start_idx == rdf_idx_helper::TABLE_END);
}


void RDFIndex::IndexIterator::start()
{
	m_cur_idx = m_start_idx;
	if (valid())
	{
		m_cur_map = bind(m_pattern, m_triples[m_cur_idx].t);
		inc_till_pattern_match();
	}
}


CodedVarMap RDFIndex::IndexIterator::current() const
{
	DBSI_CHECK_PRECOND(valid());
	DBSI_CHECK_INVARIANT(pattern_matches(m_pattern, m_triples[m_cur_idx].t));
	auto cvm = bind(m_pattern, m_triples[m_cur_idx].t);

	// if this fails then the pattern utils functions are faulty
	DBSI_CHECK_POSTCOND(cvm.has_value());

	return *cvm;
}


void RDFIndex::IndexIterator::next()
{
	DBSI_CHECK_PRECOND(valid());

#ifdef DBSI_CHECKING_INVARIANTS
	const auto last_idx = m_cur_idx;
#endif

	increment_idx();
	if (valid())
		inc_till_pattern_match();

	DBSI_CHECK_INVARIANT(last_idx != m_cur_idx);
}


bool RDFIndex::IndexIterator::valid() const
{
	DBSI_CHECK_INVARIANT(m_cur_idx < m_triples.size()
		|| m_cur_idx == rdf_idx_helper::TABLE_END);

	return m_cur_idx < rdf_idx_helper::TABLE_END;
}


void RDFIndex::IndexIterator::increment_idx()
{
#ifdef DBSI_CHECKING_INVARIANTS
	const CodedResource last_sub = m_triples[m_cur_idx].t.sub;
	const CodedResource last_obj = m_triples[m_cur_idx].t.obj;
#endif
	switch (m_eval_type)
	{
	case EvaluationType::NONE:
		m_cur_idx = rdf_idx_helper::TABLE_END;
		break;

	case EvaluationType::ALL:
		++m_cur_idx;

		if (m_cur_idx >= m_triples.size())
			m_cur_idx = rdf_idx_helper::TABLE_END;
		break;

	case EvaluationType::SP:
		/*
		* Note: this one statement hides substantial complexity,
		* as explained at length in `dbsi_rdf_index_helper.h` and
		* `TripleRowVisitor`.
		* 
		* In short, there are two possibilities when advancing
		* along the `n_sp` pointer. Either:
		* (i) the `pred` changes, in which case we go via the index
		* to obtain the next `sp` value (for the same `sub` but
		* different `pred`)
		* (ii) the `pred` remains the same, in which case it
		* is a simple pointer update.
		* 
		* Note that, crucially, (i) is constant time, because
		* `n_sp` already stores the pointer to the right element.
		*/
		m_cur_idx = std::visit(rdf_idx_helper::TripleRowVisitor(),
			m_triples[m_cur_idx].n_sp);
		DBSI_CHECK_INVARIANT(m_cur_idx == rdf_idx_helper::TABLE_END
			|| last_sub == m_triples[m_cur_idx].t.sub);
		break;

	case EvaluationType::P:
		m_cur_idx = m_triples[m_cur_idx].n_p;
		break;

	case EvaluationType::OP:
		// see comment for SP
		m_cur_idx = std::visit(rdf_idx_helper::TripleRowVisitor(),
			m_triples[m_cur_idx].n_op);
		DBSI_CHECK_INVARIANT(m_cur_idx == rdf_idx_helper::TABLE_END
			|| last_obj == m_triples[m_cur_idx].t.obj);
		break;

	default:
		DBSI_CHECK_INVARIANT(false);  // ???
		break;
	}

	if (valid())
		m_cur_map = bind(m_pattern, m_triples[m_cur_idx].t);
}


void RDFIndex::IndexIterator::inc_till_pattern_match()
{
	// increment until we finish or until we finally get a triple which matches the pattern
	while (valid() && !m_cur_map)
		increment_idx();
}


}  // namespace dbsi
