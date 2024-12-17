#include "ilp_solver_osi.hpp"

#include "OsiSolverInterface.hpp"

namespace ilp_solver
{
    ILPSolverOsi::ILPSolverOsi(OsiSolverInterface* p_ilp_solver) : d_ilp_solver(p_ilp_solver) {}

    void ILPSolverOsi::do_solve(const std::vector<double>& p_start_solution,
                                int /* p_num_threads */, bool /* p_deterministic */, int p_log_level, double /* p_max_seconds */)
    {
        auto solver = do_get_solver();
        solver->messageHandler()->setLogLevel(p_log_level);
        if (!p_start_solution.empty())
            solver->setColSolution(p_start_solution.data());
        solver->branchAndBound();
    }

    const double* ILPSolverOsi::do_get_solution() const
    {
        return do_get_solver()->getColSolution();
    }

    double ILPSolverOsi::do_get_objective() const
    {
        return do_get_solver()->getObjValue();
    }

    SolutionStatus ILPSolverOsi::do_get_status() const
    {
        const auto solver = do_get_solver();
        if (solver->isProvenOptimal())
            return SolutionStatus::PROVEN_OPTIMAL;
        else if (solver->isProvenPrimalInfeasible())
            return SolutionStatus::PROVEN_INFEASIBLE;
        else
            return (do_get_solution() == nullptr ? SolutionStatus::NO_SOLUTION
                                                 : SolutionStatus::SUBOPTIMAL);
    }
}
