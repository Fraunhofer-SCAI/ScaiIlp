#pragma once

#include "ilp_data.hpp"
#include "ilp_solver_collect.hpp"

#include <string>

namespace ilp_solver
{
    // Receives data about the ILP, writes it into shared memory,
    // and starts a new solver process that solves the ILP.
    class ILPSolverStub : public ILPSolverCollect
    {
        public:
            ILPSolverStub(const std::string& p_executable_basename, bool p_throw_on_all_crashes);

            std::vector<double>       get_solution  () const override;
            double                    get_objective () const override;
            SolutionStatus            get_status    () const override;
            double                    get_external_cpu_time_sec()    const override { return d_ilp_solution_data.cpu_time_sec; };
            double                    get_external_peak_memory_mb() const override { return d_ilp_solution_data.peak_memory; }

            void                      reset_solution()       override;

        private:
            std::string d_executable_basename;
            bool        d_throw_on_all_crashes;

            ILPSolutionData d_ilp_solution_data;

            void solve_impl() override;
    };
}
