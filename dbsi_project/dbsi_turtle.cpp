#include <cctype>
#include <sstream>
#include "dbsi_turtle.h"
#include "dbsi_assert.h"


namespace dbsi
{


/*
* Key invariants here:
* Between `start` and `next` calls, and before `read_resource` calls,
* the stream is never left pointing to whitespace
* (whitespace is always consumed until EOF or non-whitespace character)
* Each triple, if it exists, is read before the call to `current`, and
* the stream always points to the full stop at the end of the current
* triple.
* Calling `next` reads this full stop, then consumes whitespace, until
* either EOF or next tuple starts in which case it is immediately read.
*/
class TurtleTripleIterator :
	public ITripleIterator
{
public:
	TurtleTripleIterator(std::istream& in) :
		m_in(in)
	{ }

	void start() override
	{
		// seek to beginning
		m_in.seekg(0);

		// if file is nonempty, read first line and store it
		if (valid())
		{
			consume_whitespace();
			read_triple();
		}
	}

	Triple current() const override
	{
		DBSI_CHECK_PRECOND(valid());

		return m_current;
	}

	void next() override
	{
		DBSI_CHECK_PRECOND(valid());

		// finish off last tuple:
		read_end();

		// if not finished, read next triple:
		if (valid())
			read_triple();
	}

	bool valid() const override
	{
		return (bool)m_in;
	}

private:
	void read_triple()
	{
		DBSI_CHECK_INVARIANT(valid());

		m_current.sub = read_resource();

		consume_whitespace();

		m_current.pred = read_resource();

		consume_whitespace();

		m_current.obj = read_resource();

		consume_whitespace();
	}

	Resource read_resource()
	{
		DBSI_CHECK_INVARIANT(valid());
		
		// figure out what the first (necessarily non-whitespace) character is
		char c = m_in.get();
		std::stringstream out;
		out << c;

		if (c == '<')  // IRI
		{
			while ((c = m_in.get()) != '>')
			{
				// file is corrupt if this fails
				DBSI_CHECK_PRECOND((bool)m_in);

				out << c;
			}

			out << c;

			return IRI{ out.str() };
		}
		else if (c == '"')  // literal
		{
			while ((c = m_in.get()) != '"')
			{
				// file is corrupt if this fails
				DBSI_CHECK_PRECOND((bool)m_in);

				out << c;
			}

			out << c;

			return Literal{ out.str() };
		}
		else
		{
			DBSI_CHECK_PRECOND(false);  // corrupt file
		}
	}

	void read_end()
	{
		DBSI_CHECK_INVARIANT(valid());

		const char c = m_in.get();
		// file is corrupt if this fails
		DBSI_CHECK_PRECOND(c == '.');
		
		consume_whitespace();
	}

	void consume_whitespace()
	{
		// consume as much whitespace as possible,
		// but no more
		auto last_pos = m_in.tellg();
		while (m_in && std::isspace(m_in.get()))
		{
			last_pos = m_in.tellg();
		}
		m_in.seekg(last_pos);
	}

private:
	// reference must remain valid while this iterator
	// lives
	std::istream& m_in;
	Triple m_current;
};


std::unique_ptr<ITripleIterator> create_turtle_file_parser(std::istream& in)
{
	return std::make_unique<TurtleTripleIterator>(in);
}


}  // namespace dbsi
