#if WITH_OSI == 1

#include "ilp_solver_osi.hpp"

#pragma warning(push)
#pragma warning(disable : 4309) // silence warning in CBC concerning truncations of constant values in 64 bit.
#include "OsiSolverInterface.hpp"
#pragma warning(pop)

namespace ilp_solver
{
    ILPSolverOsi::ILPSolverOsi(OsiSolverInterface* p_ilp_solver)
        : d_ilp_solver(p_ilp_solver)
    {
        set_default_parameters(this);
    }


    std::vector<double> ILPSolverOsi::get_solution() const
    {
        const auto* solution_array = d_ilp_solver->getColSolution(); // Returns nullptr if no solution was found.

        if (!solution_array) return std::vector<double>();
        // No virtual call necessary, since the problem is solved.
        return std::vector<double>(solution_array, solution_array + d_ilp_solver->getNumCols());
    }


    void ILPSolverOsi::set_start_solution(const std::vector<double>& p_solution)
    {
        // get_num_variables necessary since the cache may not be included in the problem.
        assert(static_cast<int>(p_solution.size()) == get_num_variables());

        d_ilp_solver->setColSolution(p_solution.data());
    }


    double ILPSolverOsi::get_objective() const
    {
        return d_ilp_solver->getObjValue();
    }


    SolutionStatus ILPSolverOsi::get_status() const
    {
        if (d_ilp_solver->isProvenOptimal())
            return SolutionStatus::PROVEN_OPTIMAL;
        if (d_ilp_solver->isProvenPrimalInfeasible())
            return SolutionStatus::PROVEN_INFEASIBLE;
        if (d_ilp_solver->isProvenDualInfeasible())
            return SolutionStatus::PROVEN_UNBOUNDED;

        const auto* solution_array = d_ilp_solver->getColSolution();
        if (solution_array) return SolutionStatus::SUBOPTIMAL;
        return SolutionStatus::NO_SOLUTION;
    }


    void ILPSolverOsi::reset_solution()
    {
        get_solver_osi_model()->loadFromCoinModel(d_cache, false);
        d_cache_changed = false;
    }


    void ILPSolverOsi::set_num_threads(int)
    {
        // Not supported by OsiSolverInterface.
    }


    void ILPSolverOsi::set_deterministic_mode(bool)
    {
        // Not supported by OsiSolverInterface.
    }


    void ILPSolverOsi::set_log_level(int p_level)
    {
        d_ilp_solver->messageHandler()->setLogLevel(p_level);
    }


    void ILPSolverOsi::set_presolve(bool p_presolve)
    {
        // Never tested because we do not use ilp_solver_osi anywhere.
        // Thus, unsure if this is correct or enough.
        if (p_presolve)
            d_ilp_solver->setHintParam(OsiDoPresolveInInitial, true, OsiHintDo);
        else
            d_ilp_solver->setHintParam(OsiDoPresolveInInitial, false, OsiHintDo);
    }


    void ILPSolverOsi::set_max_seconds(double)
    {
        // Not supported by OsiSolverInterface.
    }

    void ILPSolverOsi::set_max_nodes(int)
    {
        // Not supported by OsiSolverInterface.
    }


    void ILPSolverOsi::set_max_solutions(int)
    {
        // Not supported by OsiSolverInterface.
    }


    void ILPSolverOsi::set_max_abs_gap(double)
    {
        // Not supported by OsiSolverInterface.
    }


    void ILPSolverOsi::set_max_rel_gap(double)
    {
        // Not supported by OsiSolverInterface.
    }


    OsiSolverInterface* ILPSolverOsi::get_solver_osi_model()
    {
        return d_ilp_solver;
    }


    void ILPSolverOsi::solve_impl()
    {
        d_ilp_solver->branchAndBound();
    }


    void ILPSolverOsi::set_objective_sense_impl(ObjectiveSense p_sense)
    {
        if (p_sense == ObjectiveSense::MINIMIZE)
            d_ilp_solver->setObjSense(1.);
        else
            d_ilp_solver->setObjSense(-1.);
    }
}

#endif
