#pragma once

#include "ilp_data.hpp"
#include "ilp_solver_collect.hpp"

#include <string>

namespace ilp_solver
{
    // Receives data about the ILP, writes it into shared memory,
    // and starts a new solver process that solves the ILP.
    class ILPSolverStub final : public ILPSolverCollect
    {
        public:
            ILPSolverStub(const std::string& p_executable_basename, bool p_throw_on_all_crashes);

            std::vector<double>       get_solution  () const override;
            double                    get_objective () const override;
            SolutionStatus            get_status    () const override;
            double                    get_external_cpu_time_sec()    const override { return d_ilp_solution_data.cpu_time_sec; };
            double                    get_external_peak_memory_mb() const override { return d_ilp_solution_data.peak_memory; }
            SolverExitCode            get_external_exit_code() const override { return d_exit_code; };

            void                      reset_solution()       override;

        private:
            const std::string d_executable_basename;
            const bool        d_throw_on_all_crashes;
            SolverExitCode    d_exit_code{SolverExitCode::ok};

            ILPSolutionData d_ilp_solution_data;

            // Runs d_executable_basename.exe.
            // Puts its exit code in d_exit_code.
            // If d_exit_code indicates a severe error or d_throw_on_all_crashes==true, in addition SolverExeException is thrown.
            // If d_exit_code indicates a known CBC problem that should be ignored silently, we test if the stub works at least with a tiny LP (function stub_tester).
            // - If that works, we keep d_exit_code, but do not throw.
            // - If that does not work, we change d_error_code and report that stub_tester does not work either.
            void solve_impl() override;
    };
}
