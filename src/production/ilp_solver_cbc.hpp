#pragma once

#ifdef WITH_CBC

#ifndef WITH_OSI
static_assert(false,
    "CBC requires the Osi-Interface and the CoinUtils contained therein. "
    "Please define WITH_OSI or deactivate CBC by not defining WITH_CBC.");
#endif

#include "ilp_solver_osi_model.hpp" // Including this also links with the required COIN Libraries.

#include <CbcModel.hpp>
#include <OsiClpSolverInterface.hpp>

class OsiSolverInterface;


namespace ilp_solver
{
    // Final implementation of CBC in ScaiILP.
    class ILPSolverCbc final : public ILPSolverOsiModel
    {
        public:
            ILPSolverCbc();

            std::vector<double> get_solution  () const override;
            double              get_objective () const override;
            SolutionStatus      get_status    () const override;

            void                reset_solution()       override;

            void set_start_solution     (ValueArray p_solution)                                    override;

            void set_num_threads        (int p_num_threads)                                        override;
            void set_deterministic_mode (bool p_deterministic)                                     override;
            void set_log_level          (int p_level)                                              override;
            void set_presolve           (bool p_presolve)                                          override;

            void set_max_nodes          (int p_nodes)                                              override;
            void set_max_solutions      (int p_solutions)                                          override;
            void set_max_abs_gap        (double p_gap)                                             override;
            void set_max_rel_gap        (double p_gap)                                             override;
            // Note: Add (or substract) epsilon to the cutoff value.
            //       CBC seems to need this to avoid (numerical) problems.
            //       We cannot add/substract epsilon inside this function,
            //       because we do not know the objective sense yet.
            void set_cutoff             (double p_cutoff)                                          override;

            void set_interim_results    (std::function<void (ILPSolutionData*)> p_interim_handler) override;

        private:
            CbcModel d_model;

            OsiSolverInterface*       get_solver_osi_model    ()       override;

            void solve_impl() override;
            void set_objective_sense_impl(ObjectiveSense p_sense) override;
            void set_max_seconds_impl(double p_seconds) override;
    };
}

#endif
