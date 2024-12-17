#pragma once

#if WITH_OSI == 1

#include "ilp_solver_osi_model.hpp"

class OsiSolverInterface;


namespace ilp_solver
{
    // Wrapper class for all ILP solvers that implement the full OsiSolverInterface
    // A few functions are not expressible in pure OsiSolverInterface terms,
    // so you may want to derive from this class instead of using it directly if your specific interface provides them.
    class ILPSolverOsi: public ILPSolverOsiModel
    {
        public:
            explicit ILPSolverOsi(OsiSolverInterface* p_ilp_solver);

            std::vector<double> get_solution            () const                                override;
            void                set_start_solution      (const std::vector<double>& p_solution) override;
            double              get_objective           () const                                override;
            SolutionStatus      get_status              () const                                override;

            void                reset_solution          ()                                      override;

            void                set_num_threads         (int p_num_threads)                     override;
            void                set_deterministic_mode  (bool p_deterministic)                  override;
            void                set_log_level           (int p_level)                           override;
            void                set_presolve            (bool p_presolve)                       override;

            void                set_max_seconds         (double p_seconds)                      override;
            void                set_max_nodes           (int p_nodes)                           override;
            void                set_max_solutions       (int p_solutions)                       override;
            void                set_max_abs_gap         (double p_gap)                          override;
            void                set_max_rel_gap         (double p_gap)                          override;

        private:
            OsiSolverInterface* d_ilp_solver;

            OsiSolverInterface* get_solver_osi_model   ()                                       override;

            void                solve_impl             ()                                      override;
            void                set_objective_sense_impl(ObjectiveSense p_sense)                override;
    };
}

#endif
