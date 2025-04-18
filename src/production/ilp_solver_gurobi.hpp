#pragma once

// Gurobi is only supported on 64-bit builds.
#if defined(WITH_GUROBI) && (_WIN64 == 1)

#include "ilp_solver_impl.hpp"


namespace ilp_solver
{
    struct _GRBmodel;
    typedef struct _GRBmodel GRBmodel;

    struct _GRBenv;
    typedef struct _GRBenv GRBenv;


    class ILPSolverGurobi final : public ILPSolverImpl
    {
        public:
            ILPSolverGurobi();
            ~ILPSolverGurobi();

            int get_num_constraints() const override;
            int get_num_variables  () const override;

            std::vector<double> get_solution () const override;
            std::vector<double> get_dual_sol () const override;
            double              get_objective() const override;
            SolutionStatus      get_status   () const override;

            void set_start_solution    (ValueArray p_solution)                  override;
            void reset_solution        ()                                       override;

            void set_num_threads       (int p_num_threads)                      override;
            void set_deterministic_mode(bool p_deterministic)                   override;
            void set_log_level         (int p_level)                            override;
            void set_presolve          (bool p_presolve)                        override;

            void set_max_nodes         (int    p_nodes)                         override;
            void set_max_solutions     (int    p_solutions)                     override;
            void set_max_abs_gap       (double p_gap)                           override;
            void set_max_rel_gap       (double p_gap)                           override;
            void set_cutoff            (double p_cutoff)                        override;

            void print_mps_file        (const std::string& p_filename)          override;

            void set_interim_results   (std::function<void(ILPSolutionData*)>)  override { /* Not yet implemented*/ }
        private:
            GRBmodel* d_model;
            GRBenv*   d_env;

            // An index shortcut.
            std::vector<int> d_indices;
            int              d_num_vars{0};
            int              d_num_cons{0};

            void add_variable_impl  (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                                     const std::string& p_name = "", OptionalValueArray p_row_values = {},
                                     OptionalIndexArray p_row_indices = {}) override;

            void add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                     const std::string& p_name = "", OptionalIndexArray p_col_indices = {}) override;

            void solve_impl              ()                       override;
            void set_objective_sense_impl(ObjectiveSense p_sense) override;
            void set_max_seconds_impl    (double p_seconds)       override;
    };
}

#endif
