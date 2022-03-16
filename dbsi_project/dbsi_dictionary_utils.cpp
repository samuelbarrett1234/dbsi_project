#include "dbsi_dictionary_utils.h"
#include "dbsi_dictionary.h"
#include "dbsi_assert.h"


namespace dbsi
{


CodedTerm encode(Dictionary& dict, const Term& t)
{
	struct TermEncoder
	{
		TermEncoder(Dictionary& dict) :
			dict(dict)
		{ }

		CodedTerm operator()(const Variable& v)
		{
			return v;
		}

		CodedTerm operator()(const Resource& r)
		{
			return dict.encode(r);
		}

		Dictionary& dict;
	};
	return std::visit(TermEncoder(dict), t);
}


Term decode(const Dictionary& dict, const CodedTerm& t)
{
	struct TermDecoder
	{
		TermDecoder(const Dictionary& dict) :
			dict(dict)
		{ }

		Term operator()(const Variable& v)
		{
			return v;
		}

		Term operator()(const CodedResource& r)
		{
			return dict.decode(r);
		}

		const Dictionary& dict;
	};
	return std::visit(TermDecoder(dict), t);
}


CodedTriple encode(Dictionary& dict, const Triple& t)
{
	return {
		dict.encode(t.sub), dict.encode(t.pred), dict.encode(t.obj)
	};
}


Triple decode(const Dictionary& dict, const CodedTriple& t)
{
	return {
		dict.decode(t.sub), dict.decode(t.pred), dict.decode(t.obj)
	};
}


CodedTriplePattern encode(Dictionary& dict, const TriplePattern& t)
{
	return {
		encode(dict, t.sub), encode(dict, t.pred), encode(dict, t.obj)
	};
}


TriplePattern decode(const Dictionary& dict, const CodedTriplePattern& t)
{
	return {
		decode(dict, t.sub), decode(dict, t.pred), decode(dict, t.obj)
	};
}


std::unique_ptr<ICodedTripleIterator> autoencode(
	Dictionary& dict, std::unique_ptr<ITripleIterator> iter)
{
	DBSI_CHECK_PRECOND(iter != nullptr);

	// simple wrapper around the given iterator which calls `encode` in `current`
	class AutoencodingTripleIterator :
		public ICodedTripleIterator
	{
	public:
		AutoencodingTripleIterator(
			Dictionary& dict, std::unique_ptr<ITripleIterator> iter) :
			m_dict(dict), m_iter(std::move(iter))
		{ }

		void start() override
		{
			m_iter->start();
		}

		CodedTriple current() const override
		{
			return encode(m_dict, m_iter->current());
		}

		void next() override
		{
			m_iter->next();
		}

		bool valid() const override
		{
			return m_iter->valid();
		}

	private:
		Dictionary& m_dict;
		std::unique_ptr<ITripleIterator> m_iter;
	};

	return std::make_unique<AutoencodingTripleIterator>(dict, std::move(iter));
}


std::unique_ptr<ITripleIterator> autodecode(
	const Dictionary& dict, std::unique_ptr<ICodedTripleIterator> iter)
{
	DBSI_CHECK_PRECOND(iter != nullptr);

	// simple wrapper around the given iterator which calls `decode` in `current`
	class AutodecodingTripleIterator :
		public ITripleIterator
	{
	public:
		AutodecodingTripleIterator(
			const Dictionary& dict, std::unique_ptr<ICodedTripleIterator> iter) :
			m_dict(dict), m_iter(std::move(iter))
		{ }

		void start() override
		{
			m_iter->start();
		}

		Triple current() const override
		{
			return decode(m_dict, m_iter->current());
		}

		void next() override
		{
			m_iter->next();
		}

		bool valid() const override
		{
			return m_iter->valid();
		}

	private:
		const Dictionary& m_dict;
		std::unique_ptr<ICodedTripleIterator> m_iter;
	};

	return std::make_unique<AutodecodingTripleIterator>(dict, std::move(iter));
}


}  // namespace dbsi
