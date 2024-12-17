// definiert Funktion init_unit_test bzw init_unit_test_suite
// #define BOOST_TEST_MAIN muss vor #include <boost/test/unit_test.hpp> stehen.
// Deshalb darf diese Datei nicht mit Precompiled Header uebersetzt werden.
#define BOOST_TEST_MAIN


#include "tester.hpp"
#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include <boost/nowide/filesystem.hpp>
#include <boost/test/unit_test.hpp>

int main(int p_argc, char* p_argv[])
{
    // Make boost::filesystem treat all char-based strings as UTF-8.
    boost::nowide::nowide_filesystem();
    return unit_test_main(init_unit_test_suite, p_argc, p_argv);
}
