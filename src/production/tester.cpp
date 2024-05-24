#include "tester.hpp"

#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

constexpr double c_eps = 0.0001;

namespace ilp_solver
{
struct ExceptionTesterImpl : ExceptionTester
{
    void throw_exception(const std::string& p_message) const override { throw std::runtime_error(p_message); }
};


extern "C" ExceptionTester* __stdcall create_exception_tester()
{
    return new ExceptionTesterImpl();
}


extern "C" void __stdcall destroy_exception_tester(ExceptionTester* p_exception)
{
    delete p_exception;
}


SolverExitCode stub_tester(const std::string& p_executable_basename)
{
    ilp_solver::ScopedILPSolver solver;
    try
    {
        solver = create_solver_stub(p_executable_basename.c_str(), true);
        solver->set_max_seconds(5);

        // max x+y, -1 <= x, y <= 1
        solver->add_variable_continuous(1, -1, 1);
        solver->add_variable_continuous(1, -1, 1);

        // x+2y <= 2
        std::vector<double> values_1{1, 2};
        solver->add_constraint_upper(values_1, 2);

        // 2x+y <= 2
        std::vector<double> values_2{2, 1};
        solver->add_constraint_upper(values_2, 2);

        solver->maximize();
        const auto solution = solver->get_solution();

        std::vector<double> expected_solution{2. / 3., 2. / 3.};
        auto equal_eps = [](double p_value_1, double p_value_2) { return fabs(p_value_1 - p_value_2) <= c_eps; };

        if (solver->get_status() != SolutionStatus::PROVEN_OPTIMAL)
            return SolverExitCode::stub_tester_failed;
        if (!equal_eps(solution[0], expected_solution[0]) || !equal_eps(solution[1], expected_solution[1]))
            return SolverExitCode::stub_tester_failed;

        return solver->get_external_exit_code();
    }
    catch (...)
    {
        if (!solver || solver->get_external_exit_code() == SolverExitCode::ok)
            return SolverExitCode::stub_tester_failed;
        return solver->get_external_exit_code();
    }
}
} // namespace ilp_solver
