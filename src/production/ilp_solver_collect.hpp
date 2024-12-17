#ifndef _ILP_SOLVER_COLLECT_HPP
#define _ILP_SOLVER_COLLECT_HPP

#include "ilp_data.hpp"
#include "ilp_solver_interface_impl.hpp"

namespace ilp_solver
{
    // Stores all information about the ILP and the solver.
    class ILPSolverCollect : public ILPSolverInterfaceImpl
    {
        private:
            ILPData d_ilp_data;

            virtual void do_solve(const ILPData& p_data) = 0;

            void do_add_variable   (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& /* p_name */, VariableType p_type) override;
            void do_add_constraint (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const std::string& /* p_name */)                                          override;

            void do_set_objective_sense (ObjectiveSense p_sense) override;
            void do_prepare_and_solve   (int p_num_threads, bool p_deterministic, int p_log_level, double p_max_seconds) override;
    };
}

#endif
