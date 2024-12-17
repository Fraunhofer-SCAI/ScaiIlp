// definiert Funktion init_unit_test bzw init_unit_test_suite
// #define BOOST_TEST_MAIN muss vor #include <boost/test/unit_test.hpp> stehen.
// Deshalb darf diese Datei nicht mit Precompiled Header uebersetzt werden.
#define BOOST_TEST_MAIN


#include "ilp_solver_exception.hpp"
#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include <boost/test/unit_test.hpp>
#include <functional>
#include <string>
#include <vector>

const auto c_solver_exe_name = "ScaiIlpExe.exe";


int main(int p_argc, char* p_argv[])
{
    return  unit_test_main(init_unit_test_suite, p_argc, p_argv );
}
