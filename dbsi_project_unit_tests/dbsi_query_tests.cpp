#include <sstream>
#include <boost/test/unit_test.hpp>
#include "dbsi_query.h"


namespace dbsi
{


bool operator== (const EmptyQuery& q1, const EmptyQuery& q2)
{
    return true;
}


template<typename T>
bool operator== (const EmptyQuery& q1, const T& q2)
{
    return false;
}


bool operator== (const QuitQuery& q1, const QuitQuery& q2)
{
    return true;
}


template<typename T>
bool operator== (const QuitQuery& q1, const T& q2)
{
    return false;
}


bool operator==(const LoadQuery& q1, const LoadQuery& q2)
{
    return q1.filename == q2.filename;
}


template<typename T>
bool operator== (const LoadQuery& q1, const T& q2)
{
    return false;
}


bool operator==(const CountQuery& q1, const CountQuery& q2)
{
    return q1.match == q2.match;
}


template<typename T>
bool operator== (const CountQuery& q1, const T& q2)
{
    return false;
}


bool operator==(const SelectQuery& q1, const SelectQuery& q2)
{
    return q1.match == q2.match && q1.projection == q2.projection;
}


template<typename T>
bool operator== (const SelectQuery& q1, const T& q2)
{
    return false;
}


bool operator==(const BadQuery& q1, const BadQuery& q2)
{
    return q1.error == q2.error;
}


template<typename T>
bool operator== (const BadQuery& q1, const T& q2)
{
    return false;
}


}  // namespace dbsi


using namespace dbsi;


// override ostream for queries
namespace std
{


ostream& operator<<(ostream& os, const EmptyQuery& q)
{
    return os << "EmptyQuery()";
}


ostream& operator<<(ostream& os, const QuitQuery& q)
{
    return os << "QuitQuery()";
}


ostream& operator<<(ostream& os, const BadQuery& q)
{
    return os << "BadQuery(\"" << q.error << "\")";
}


ostream& operator<<(ostream& os, const LoadQuery& q)
{
    return os << "LoadQuery(\"" << q.filename << "\")";
}


ostream& operator<<(ostream& os, const SelectQuery& q)
{
    return os << "SelectQuery(\"" << q.match.size() << ',' << q.projection.size() << ')';
}


ostream& operator<<(ostream& os, const CountQuery& q)
{
    return os << "CountQuery(" << q.match.size() << ')';
}


ostream& operator<<(ostream& os, const AnyQuery& q)
{
    return std::visit([&os](auto&& q) -> ostream& { return os << q; }, q);
}

}  // namespace std


BOOST_AUTO_TEST_SUITE(QueryTests);


BOOST_AUTO_TEST_CASE(TestEmpty)
{
    std::stringstream query(
        ""
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<EmptyQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_CASE(TestQuit)
{
    std::stringstream query(
        "QUIT"
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<QuitQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_CASE(TestLoad)
{
    std::stringstream query(
        "LOAD myfolder/myfile.txt"
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<LoadQuery>(&result) != nullptr);
    BOOST_CHECK_EQUAL(std::get<LoadQuery>(result), LoadQuery{ "myfolder/myfile.txt" });
}


BOOST_AUTO_TEST_CASE(TestBadCommand)
{
    std::stringstream query(
        "BADCOMMAND"
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<BadQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_CASE(TestLeadingWhitespace)
{
    std::stringstream query(
        " \n  \tQUIT"
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<QuitQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_CASE(TestWhitespaceAfterwards)
{
    std::stringstream query(
        "QUIT \n  \t"
    );
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<QuitQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_CASE(TestEOS)
{
    std::stringstream query(
        "QUIT"
    );
    parse_query(query);
    auto result = parse_query(query);
    BOOST_REQUIRE(std::get_if<EmptyQuery>(&result) != nullptr);
}


BOOST_AUTO_TEST_SUITE_END();  // QueryTests
