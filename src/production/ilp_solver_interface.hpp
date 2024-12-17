#ifndef _ILP_SOLVER_INTERFACE_HPP
#define _ILP_SOLVER_INTERFACE_HPP

#include <string>
#include <vector>

namespace ilp_solver
{
    enum class SolutionStatus { PROVEN_OPTIMAL, PROVEN_INFEASIBLE, SUBOPTIMAL, NO_SOLUTION };


    class SolverExeException : public std::runtime_error
    {
    public:
        explicit SolverExeException (const std::string& p_what) : std::runtime_error(p_what) {};
    };


    class ILPSolverInterface
    {
        public:
            virtual void add_variable_boolean    (                                                                                double p_objective,                                             const std::string& p_name = "") = 0;
            virtual void add_variable_boolean    (                                       const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") = 0;
            virtual void add_variable_boolean    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") = 0;
            virtual void add_variable_integer    (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_integer    (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_integer    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_continuous (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_continuous (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_continuous (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;

            virtual void add_constraint          (                                       const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") = 0;  // l <= a*x <= r
            virtual void add_constraint          (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") = 0;  // l <= a*x <= r
            virtual void add_constraint_upper    (                                       const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") = 0;  //      a*x <= r
            virtual void add_constraint_upper    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") = 0;  //      a*x <= r
            virtual void add_constraint_lower    (                                       const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") = 0;  // l <= a*x
            virtual void add_constraint_lower    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") = 0;  // l <= a*x
            virtual void add_constraint_equality (                                       const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") = 0;  //      a*x = v
            virtual void add_constraint_equality (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") = 0;  //      a*x = v

            virtual void set_start_solution      (const std::vector<double>& p_solution) = 0;

            virtual void                      minimize      ()       = 0;
            virtual void                      maximize      ()       = 0;
            virtual const std::vector<double> get_solution  () const = 0;
            virtual double                    get_objective () const = 0;
            virtual SolutionStatus            get_status    () const = 0;

            virtual void set_num_threads        (int p_num_threads)    = 0;
            virtual void set_deterministic_mode (bool p_deterministic) = 0;
            virtual void set_log_level          (int p_level)          = 0; // 0: no output; verbosity increases with p_level
            virtual void set_max_seconds        (double p_seconds)     = 0;

            virtual ~ILPSolverInterface() {}
    };
}

#endif
