#include "dbsi_rdf_index.h"
#include "dbsi_pattern_utils.h"
#include "dbsi_assert.h"


namespace dbsi
{


void RDFIndex::add(CodedTriple t)
{
	m_triples.push_back(std::move(t));
}


std::unique_ptr<ICodedVarMapIterator> RDFIndex::evaluate(
	CodedTriplePattern pattern, std::optional<TripleOrder> request_output_order) const
{
	class NaiveCodedVarMapIterator :
		public ICodedVarMapIterator
	{
	public:
		NaiveCodedVarMapIterator(const std::vector<CodedTriple>& triples, CodedTriplePattern pat) :
			m_triples(triples), m_index(triples.size()), m_pat(pat)
		{ }

		void start() override
		{
			m_index = 0;

			while (valid() && !pattern_matches(m_pat, m_triples[m_index]))
				++m_index;
		}

		CodedVarMap current() const override
		{
			DBSI_CHECK_PRECOND(valid());

			auto result = bind(m_pat, m_triples[m_index]);

			// if this fails then the `pattern_matches` function is faulty
			DBSI_CHECK_POSTCOND(result.has_value());

			return *result;
		}

		void next() override
		{
			DBSI_CHECK_PRECOND(valid());

			++m_index;

			while (valid() && !pattern_matches(m_pat, m_triples[m_index]))
				++m_index;
		}

		bool valid() const override
		{
			return m_index < m_triples.size();
		}

	private:
		const std::vector<CodedTriple>& m_triples;
		CodedTriplePattern m_pat;
		size_t m_index;
	};

	return std::make_unique<NaiveCodedVarMapIterator>(m_triples, pattern);
}


}  // namespace dbsi
