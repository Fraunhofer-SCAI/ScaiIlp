#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"
#include "ilp_solver_interface_t.hpp"
#include "serialization_t.hpp"

#include <functional>
#include <string>
#include <vector>

const auto c_solver_exe_name = "ScaiIlpExe.exe";

namespace ilp_solver
{
    static void run_tests(std::function<ILPSolverInterface*()> p_create_solver, const std::string& p_solver_name)
    {
        typedef void (*SolverTest)(ILPSolverInterface* p_solver, const std::string& p_solver_name);
        std::vector<SolverTest> tests;
        tests.push_back(test_sorting);
        tests.push_back(test_linear_programming);
        tests.push_back(test_start_solution_minimization);
        tests.push_back(test_start_solution_maximization);
        tests.push_back(test_performance);

        for (const auto test: tests)
        {
            auto solver = p_create_solver();
            //solver->set_log_level(1);
            execute_test_and_destroy_solver(solver, p_solver_name, test);
        }
    }
}

void main()
{
    test_serialization();

    auto cbc_stub_solver_generator = []() { return ilp_solver::create_solver_stub(c_solver_exe_name); };

    ilp_solver::run_tests(ilp_solver::create_solver_cbc, "Cbc Solver");
    ilp_solver::run_tests(cbc_stub_solver_generator, "Cbc Stub Solver");

    execute_test_and_destroy_solver(cbc_stub_solver_generator(), c_solver_exe_name, ilp_solver::test_bad_alloc);
}
