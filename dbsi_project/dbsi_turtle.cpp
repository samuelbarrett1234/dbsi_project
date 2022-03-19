#include <cctype>
#include <list>
#include <iostream>
#include "dbsi_turtle.h"
#include "dbsi_assert.h"
#include "dbsi_parse_helper.h"


namespace dbsi
{


/*
* Key invariants here:
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
		m_in(in),
		m_error(false)
	{
		m_in >> std::noskipws;
	}

	void start() override
	{
		// seek to beginning
		m_in.seekg(0);

		// reset error flag if necessary
		m_error = false;

		// if file is nonempty, read first line and store it
		if (valid())
		{
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
		return (bool)m_in && !m_error;
	}

private:
	void read_triple()
	{
		DBSI_CHECK_INVARIANT(valid());

		m_in >> std::ws;
		if (!m_in)
			return;  // EOF, so don't carry on, but this is not an error

		// read subject
		auto maybe_resource = parse_resource(m_in);
		if (!maybe_resource)
		{
			set_error("subject");
			return;
		}
		m_current.sub = *maybe_resource;

		// read predicate
		maybe_resource = parse_resource(m_in);
		if (!maybe_resource)
		{
			set_error("predicate");
			return;
		}
		m_current.pred = *maybe_resource;

		// read object
		maybe_resource = parse_resource(m_in);
		if (!maybe_resource)
		{
			set_error("object");
			return;
		}
		m_current.obj = *maybe_resource;
	}

	void read_end()
	{
		DBSI_CHECK_INVARIANT(valid());

		m_in >> std::ws;

		const char c = m_in.get();

		// file is corrupt if this fails
		if (c != '.')
		{
			set_error("triple delimiter");
			return;
		}
	}

	void set_error(const char* what_is_invalid)
	{
		m_error = true;
		std::cerr << "Encountered an invalid "
			<< what_is_invalid
			<< " while loading. Stopping parsing the file. Current file stream position is "
			<< m_in.tellg()
			<< std::endl;
	}

private:
	// reference must remain valid while this iterator
	// lives
	std::istream& m_in;
	bool m_error;
	Triple m_current;
};


std::unique_ptr<ITripleIterator> create_turtle_file_parser(std::istream& in)
{
	return std::make_unique<TurtleTripleIterator>(in);
}


}  // namespace dbsi
