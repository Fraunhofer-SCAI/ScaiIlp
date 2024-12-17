#if WITH_CBC == 1

// Link with the required CBC Libraries.
#pragma comment(lib, "libCbc.lib")
#pragma comment(lib, "libClp.lib")
#pragma comment(lib, "libCgl.lib")

#include "ilp_solver_cbc.hpp"

#include "ilp_data.hpp"

#include "CglTreeInfo.hpp" // Needed to deal with the probing_info memory leak in Cbc
#pragma warning(push)
#pragma warning(disable : 5033) // silence warning in CBC concerning the deprecated keyword 'register'
#include "CoinMessageHandler.hpp"
#include "OsiSolverInterface.hpp"
#pragma warning(pop)

#include <algorithm>


namespace ilp_solver
{
    class InterimEventHandler : public CbcEventHandler
    {
    public:
        CbcAction event(CbcEvent whichevent) override;
        CbcEventHandler* clone() const;

        InterimEventHandler(std::function<void (ILPSolutionData*)> p_interim_handler) : d_interim_handler{std::move(p_interim_handler)} {}
    private:
         std::function<void (ILPSolutionData*)> d_interim_handler;
         ILPSolutionData                        d_last_solution;
    };


    CbcEventHandler::CbcAction InterimEventHandler::event(CbcEvent p_whichevent)
    {
        if (p_whichevent == CbcEvent::solution || p_whichevent == CbcEvent::heuristicSolution)
        {
            const double* best_solution = model_->bestSolution();
            assert(best_solution);

            auto new_value = model_->getObjValue();
            if (d_last_solution.solution_status == SolutionStatus::NO_SOLUTION
                || (model_->getObjSense() * d_last_solution.objective > model_->getObjSense() * new_value))
            {
                auto size = model_->getNumCols();
                d_last_solution.solution.assign(best_solution, best_solution + size);
                d_last_solution.objective = new_value;
                d_last_solution.solution_status = SolutionStatus::SUBOPTIMAL;
                d_interim_handler(&d_last_solution);
            }
        }
        return CbcAction::noAction;
    }


    CbcEventHandler* InterimEventHandler::clone() const
    {
        return static_cast<CbcEventHandler*>(new InterimEventHandler(*this));
    }


    ILPSolverCbc::ILPSolverCbc()
    {
        // CbcModel assumes ownership over solver and deletes it in its destructor.
        OsiSolverInterface* solver = new OsiClpSolverInterface();

        // Output should come from CBC, not from CBCs solver.
        solver->messageHandler()->setLogLevel(0);
        solver->setHintParam(OsiDoReducePrint, 1);

        d_model.assignSolver(solver, true);
        set_default_parameters(this);
    }


    std::vector<double> ILPSolverCbc::get_solution() const
    {
        // The best solution is stored by CbcModel, not by the solver, thus reimplementation.
        const auto* result = d_model.bestSolution();
        if (!result) return std::vector<double>();
        // No virtual call necessary, since the problem is solved.
        return std::vector<double>(result, result + d_model.getNumCols());
    }


    double ILPSolverCbc::get_objective() const
    {
        // The best objective value is stored by CbcModel, not by the solver, thus reimplementation.
        return d_model.getObjValue();
    }


    SolutionStatus ILPSolverCbc::get_status() const
    {
        // Solution status is stored by CbcModel.
        if (d_model.isProvenOptimal())
            return SolutionStatus::PROVEN_OPTIMAL;
        else if (d_model.isProvenInfeasible())
            return SolutionStatus::PROVEN_INFEASIBLE;
        else if (d_model.isProvenDualInfeasible())
            return SolutionStatus::PROVEN_UNBOUNDED;
        else
            return ((d_model.bestSolution() == nullptr) ? SolutionStatus::NO_SOLUTION
                                                        : SolutionStatus::SUBOPTIMAL);
    }


    void ILPSolverCbc::reset_solution()
    {
        d_model.gutsOfDestructor2(); // "Clears enough to reset CbcModel as if no branch and bound done."
        d_model.solver()->loadFromCoinModel(d_cache, false);
        d_cache_changed = false;
    }


    void ILPSolverCbc::set_start_solution(const std::vector<double>& p_solution)
    {
        // make sure that the cache was integrated
        prepare_impl();
        assert( static_cast<int>(p_solution.size()) == get_num_variables() );
        // Set the current best solution of Cbc to the given solution, check for feasibility, but not for better objective value.
        d_model.setBestSolution(p_solution.data(), static_cast<int>(p_solution.size()), COIN_DBL_MAX, true);
    }


    void ILPSolverCbc::set_num_threads(int p_num_threads)
    {
        const auto cbc_num_threads = (p_num_threads == 1 ? 0 : p_num_threads); // peculiarity of Cbc (1 is 'for testing').
        d_model.setNumberThreads(cbc_num_threads);
    }


    void ILPSolverCbc::set_deterministic_mode(bool p_deterministic)
    {
        const auto cbc_thread_mode = ((d_model.getNumberThreads() > 1 && p_deterministic) ? 1 : 0);
        d_model.setThreadMode(cbc_thread_mode);
    }


    void ILPSolverCbc::set_log_level(int p_level)
    {
        int level = std::clamp(p_level, 0, 4);          // log level must be between 0 and 4
        d_model.messageHandler()->setLogLevel(level);
    }


    void ILPSolverCbc::set_presolve(bool p_presolve)
    {
        if (p_presolve)
            d_model.setTypePresolve(1);
        else
            d_model.setTypePresolve(0);
    }


    void ILPSolverCbc::set_max_seconds(double p_seconds)
    {
        d_model.setMaximumSeconds(p_seconds);
    }


    void ILPSolverCbc::set_max_nodes(int p_nodes)
    {
        d_model.setMaximumNodes(p_nodes);
    }


    void ILPSolverCbc::set_max_solutions(int p_solutions)
    {
        d_model.setMaximumSolutions(p_solutions);
    }


    void ILPSolverCbc::set_max_abs_gap(double p_abs_gap)
    {
        d_model.setAllowableGap(p_abs_gap);
    }


    void ILPSolverCbc::set_max_rel_gap(double p_rel_gap)
    {
        d_model.setAllowableFractionGap(p_rel_gap);
    }


    void ILPSolverCbc::set_cutoff(double p_cutoff)
    {
        // Only set cutoff if intended. Otherwise stick to the CBC default.
        if (p_cutoff != c_default_cutoff)
            d_model.setCutoff(p_cutoff);
    }


    void ILPSolverCbc::set_interim_results(std::function<void (ILPSolutionData*)> p_interim_handler)
    {
        InterimEventHandler handler{ p_interim_handler };
        d_model.passInEventHandler(&handler); // CBC clones the handler, so no dangling pointer.
    }


    OsiSolverInterface* ILPSolverCbc::get_solver_osi_model()
    {
        return d_model.solver();
    }


    void ILPSolverCbc::solve_impl()
    {
        // The probingInfo is not deleted on successive solves, but overwritten.
        // Thus, it produces memory leaks if not deleted here.
        auto probing_ptr = d_model.probingInfo();
        if (probing_ptr)
            delete probing_ptr;

        d_model.initialSolve();
        d_model.branchAndBound();
    }


    void ILPSolverCbc::set_objective_sense_impl(ObjectiveSense p_sense)
    {
        double sense{ (p_sense == ObjectiveSense::MINIMIZE) ? 1.0 : -1.0 };
        d_model.setObjSense(sense);

        // If we have a current solution, we recompute the objective value.
        // This may be necessary because of two reasons,
        //     the best solution may have been set before the cache was integrated
        //     the setObjSense function somehow manipulates the current objective value incorrectly.
        // This is called after prepare and before the solve,
        // so this is always the correct best objective value found.
        double* sol = d_model.bestSolution();
        if (sol)
        {
            const double* coeff = d_model.solver()->getObjCoefficients();
            double          obj = 0.;
            // No virtual call necessary, since prepare is called beforehand.
            for (int i = 0; i < d_model.getNumCols(); i++)
                obj += coeff[i] * sol[i];
            d_model.setObjValue(obj);
        }
    }
}

#endif
