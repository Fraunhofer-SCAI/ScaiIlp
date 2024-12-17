#ifndef _ILP_SOLVER_STUB_HPP
#define _ILP_SOLVER_STUB_HPP

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
            explicit ILPSolverStub(const std::string& p_executable_basename);

            std::vector<double>       get_solution  () const override;
            double                    get_objective () const override;
            SolutionStatus            get_status    () const override;

            void                      reset_solution()       override;

        private:
            std::string d_executable_basename;

            ILPSolutionData d_ilp_solution_data;

            void solve_impl() override;
    };
}

#endif
