#ifndef _ILP_SOLVER_INTERFACE_IMPL_HPP
#define _ILP_SOLVER_INTERFACE_IMPL_HPP

#include "ilp_solver_interface.hpp"

namespace ilp_solver
{
    enum class VariableType   { INTEGER, CONTINUOUS };
    enum class ObjectiveSense { MINIMIZE, MAXIMIZE };

    class ILPSolverInterfaceImpl : public ILPSolverInterface
    {
        public:
            void add_variable_boolean    (                                                                                double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_boolean    (                                       const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_boolean    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_integer    (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_integer    (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_integer    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;

            void add_constraint          (                                       const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint          (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_upper    (                                       const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_upper    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_lower    (                                       const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override;
            void add_constraint_lower    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override;
            void add_constraint_equality (                                       const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") override;
            void add_constraint_equality (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") override;

            void set_start_solution      (const std::vector<double>& p_solution) override;

            void                      minimize      ()       override;
            void                      maximize      ()       override;
            const std::vector<double> get_solution  () const override;
            double                    get_objective () const override;
            SolutionStatus            get_status    () const override;

            void set_num_threads        (int p_num_threads)    override;
            void set_deterministic_mode (bool p_deterministic) override;
            void set_log_level          (int p_level)          override;
            void set_max_seconds        (double p_seconds)     override;

        protected:
            ILPSolverInterfaceImpl();

        private:
            std::vector<int>    d_all_col_indices;
            std::vector<int>    d_all_row_indices;
            std::vector<double> d_start_solution;

            int    d_num_threads;
            bool   d_deterministic;
            int    d_log_level;
            double d_max_seconds;

            virtual void do_add_variable   (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective,
                                            double p_lower_bound, double p_upper_bound, const std::string& p_name, VariableType p_type)             = 0;
            virtual void do_add_constraint (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound,
                                            double p_upper_bound, const std::string& p_name)                                                        = 0;

            virtual void           do_set_objective_sense (ObjectiveSense p_sense) = 0;
            virtual void           do_prepare_and_solve   (const std::vector<double>& p_start_solution,                                          // p_start_solution can be empty;
                                                           int p_num_threads, bool p_deterministic, int p_log_level, double p_max_seconds) = 0;
            virtual const double*  do_get_solution        () const = 0;
            virtual double         do_get_objective       () const = 0;
            virtual SolutionStatus do_get_status          () const = 0;

            void add_variable_and_update_index_vector   (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values,
                                                         double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name, VariableType p_type);
            void add_constraint_and_update_index_vector (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,
                                                         double p_lower_bound, double p_upper_bound, const std::string& p_name);
            void solve();
    };
}

#endif
