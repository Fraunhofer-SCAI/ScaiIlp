#include "ilp_solver_exception.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>


using std::endl;
using std::string;

namespace ilp_solver
{
    void test_create_exception()
    {
        const auto exception_message = string("This is a test exception.");
        const auto exception = ilp_solver::create_exception();
        try
        {
            exception->throw_exception(exception_message);
            destroy_exception(exception);
            BOOST_FAIL("Failed to throw created test exception.");

        }
        catch (std::exception& e)
        {
            const auto exception_message_2 = string(e.what());
            destroy_exception(exception);
            BOOST_REQUIRE_EQUAL(exception_message_2, exception_message);
        }
    }
}

BOOST_AUTO_TEST_SUITE( IlpSolverExceptionT );

BOOST_AUTO_TEST_CASE ( Exception )
{
    ilp_solver::test_create_exception ();
}

BOOST_AUTO_TEST_SUITE_END();
