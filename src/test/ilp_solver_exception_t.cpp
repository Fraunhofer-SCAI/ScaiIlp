#include "ilp_solver_exception.hpp"

#include <boost/test/unit_test.hpp>
#include <string>

namespace ilp_solver
{
    void test_create_exception()
    {
        const auto exception_message = std::string("This is a test exception.");
        const auto exception_tester = ilp_solver::create_exception_tester();
        try
        {
            exception_tester->throw_exception(exception_message);
            destroy_exception_tester(exception_tester);
            BOOST_FAIL("Failed to throw created test exception.");
        }
        catch (const std::exception& e)
        {
            destroy_exception_tester(exception_tester);
            BOOST_REQUIRE_EQUAL(e.what(), exception_message);
        }
    }
}

BOOST_AUTO_TEST_SUITE( IlpSolverExceptionT );

BOOST_AUTO_TEST_CASE ( Exception )
{
    ilp_solver::test_create_exception();
}

BOOST_AUTO_TEST_SUITE_END();
