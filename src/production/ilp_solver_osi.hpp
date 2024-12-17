#ifndef _ILP_SOLVER_OSI_HPP
#define _ILP_SOLVER_OSI_HPP

#include "ilp_solver_osi_model.hpp"

class OsiSolverInterface;

namespace ilp_solver
{
    // Wrapper class for all ILP solvers that implement the OsiSolverInterface
    class ILPSolverOsi: public ILPSolverOsiModel
    {
        public:
            explicit ILPSolverOsi(OsiSolverInterface* p_ilp_solver);

        private:
            OsiSolverInterface* d_ilp_solver;

            OsiSolverInterface*       do_get_solver    ()       override { return d_ilp_solver; }
            const OsiSolverInterface* do_get_solver    () const override { return d_ilp_solver; }

            void                      do_solve         (const std::vector<double>& p_start_solution, int p_num_threads, bool p_deterministic, int p_log_level, double p_max_seconds) override;
            const double*             do_get_solution  () const override;
            double                    do_get_objective () const override;
            SolutionStatus            do_get_status    () const override;
    };
}

#endif
