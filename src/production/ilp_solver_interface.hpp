#ifndef _ILP_SOLVER_INTERFACE_HPP
#define _ILP_SOLVER_INTERFACE_HPP

#include <string>
#include <vector>

namespace ilp_solver
{
    enum class SolutionStatus { PROVEN_OPTIMAL, PROVEN_INFEASIBLE, PROVEN_UNBOUNDED, SUBOPTIMAL, NO_SOLUTION };


    static constexpr int    c_default_num_threads  { 1 };
    static constexpr int    c_default_log_level    { 0 };
    static constexpr bool   c_default_deterministic{ true };
    static constexpr bool   c_default_presolve     { true };

    static constexpr double c_default_max_seconds  { std::numeric_limits<double>::max() };
    static constexpr int    c_default_max_solutions{ std::numeric_limits<int>::max() };
    static constexpr int    c_default_max_nodes    { std::numeric_limits<int>::max() };
    static constexpr double c_default_max_abs_gap  { 0. };
    static constexpr double c_default_max_rel_gap  { 0. };

    constexpr double c_pos_inf_bound{ std::numeric_limits<double>::max() / 2 };
    constexpr double c_neg_inf_bound{ std::numeric_limits<double>::lowest() / 2 };
    constexpr double c_pos_inf      { std::numeric_limits<double>::max() };
    constexpr double c_neg_inf      { std::numeric_limits<double>::lowest() };


    class SolverExeException : public std::runtime_error
    {
    public:
        explicit SolverExeException (const std::string& p_what) : std::runtime_error(p_what) {};
    };

    // This class is the basic interface fulfilled by all ScaiILP solver classes.
    // Please derive a new solver implementation from ILPSolverImpl instead of this class, to avoid redundant work.
    class ILPSolverInterface
    {
        public:
            // Add a [boolean | integer | continuous] variable to the linear system.
            // The name is optional.
            // The cost of the variable in the objective function is its value times the factor p_objective.
            // The constraints on the variable are either
            //     not participating in any current constraints.
            //     the factor p_row_values[i] in each current constraint i                (i < |constraints|)
            //     the factor p_row_values[i] in each current constraint p_row_indices[i] (i < p_row_indices.size()).
            virtual void add_variable_boolean    (                                                                                double p_objective,                                             const std::string& p_name = "") = 0;
            virtual void add_variable_boolean    (                                       const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") = 0;
            virtual void add_variable_boolean    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") = 0;

            virtual void add_variable_integer    (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_integer    (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_integer    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;

            virtual void add_variable_continuous (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_continuous (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;
            virtual void add_variable_continuous (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") = 0;


            // Add an [general | upper | lower | equality ] constraint to the linear system.
            // The name is optional.
            // The constraint can include either all or some current variables.
            //     the constraint has the form [p_lower_bound <= ]  p_col_values^T current_variables [ <= p_upper_bound]  ( or = p_value for equality).
            //     all indices that are not occurring in p_col_indices are treated as if their corresponding p_col_values value is 0. (i.e. as if the variable does not participate in the constraint.)
            virtual void add_constraint          (                                       const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") = 0;  // l <= a*x <= r
            virtual void add_constraint          (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") = 0;  // l <= a*x <= r

            virtual void add_constraint_upper    (                                       const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") = 0;  //      a*x <= r
            virtual void add_constraint_upper    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") = 0;  //      a*x <= r

            virtual void add_constraint_lower    (                                       const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") = 0;  // l <= a*x
            virtual void add_constraint_lower    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") = 0;  // l <= a*x

            virtual void add_constraint_equality (                                       const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") = 0;  //      a*x = v
            virtual void add_constraint_equality (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") = 0;  //      a*x = v

            // Obtain the current number of [constraints | variables].
            virtual int get_num_constraints() const = 0;
            virtual int get_num_variables()   const = 0;

            // Set a starting solution.
            // Depending on the solver, it may be checked whether the solution is actually valid or not.
            virtual void set_start_solution      (const std::vector<double>& p_solution) = 0;

            // [Minimize | Maximize] the currently given objective function under the given constraints.
            virtual void                      minimize      ()       = 0;
            virtual void                      maximize      ()       = 0;

            // Obtain a vector-copy of the best solution found.
            virtual std::vector<double>       get_solution  () const = 0;

            // Obtain the value of the objective function of the best solution found.
            virtual double                    get_objective () const = 0;

            // Obtain the current status of the solver.
            virtual SolutionStatus            get_status    () const = 0;

            // Delete all information about previous solutions while keeping the model and settings.
            virtual void                      reset_solution()       = 0;

            // Set the maximum number of threads used during the solve.
            // May be unsupported by some solvers.
            virtual void set_num_threads        (int p_num_threads)    = 0;

            // Set whether the solve should be deterministic.
            // May be unsupported by some solvers.
            virtual void set_deterministic_mode (bool p_deterministic) = 0;

            // Set the logging level of the given solver.
            // 0 means that the solver should not have any output.
            // Otherwise, the verbosity increases with p_level.
            // May be unsupported by some solvers.
            virtual void set_log_level          (int p_level)          = 0;

            // Enables or disables preprocessing and presolve directives of the solver.
            // May be unsupported by some solvers.
            // true:  on
            // false: off
            virtual void set_presolve           (bool p_presolve)      = 0;

            // Set the number of seconds after which the solver should terminate.
            // This may be not followed exactly. The duration may be slightly longer than the given number.
            // May be unsupported by some solvers.
            virtual void set_max_seconds        (double p_seconds)     = 0;

            // Set the maximum number of nodes in the branch & bound tree after which the solver should terminate.
            // What exactly constitutes a node may be solver-dependent.
            // May be unsupported by some solvers.
            virtual void set_max_nodes          (int p_nodes)          = 0;

            // Set the maximum number of solutions found after which the solver should terminate.
            // What exactly constitutes a solution may be solver-dependent.
            // May be unsupported by some solvers.
            virtual void set_max_solutions      (int p_solutions)      = 0;

            // Instructs the solver to stop when the gap between the objective value of the best known solution
            // and the current best bound on any solution is less than p_gap.
            // May be unsupported by some solvers.
            virtual void set_max_abs_gap        (double p_gap)         = 0;

            // Instructs the solver to stop when the gap between the objective value of the best known solution
            // and the current best bound on any solution is less than this fraction of the absolute value
            // of the best known solution.
            // The computation of the relative gap may be solver-dependent.
            // May be unsupported by some solvers.
            virtual void set_max_rel_gap        (double p_gap)         = 0;

            // Print a mps-formatted file of the current model.
            // p_path must be valid path to a file with write-permission.
            // Not const because some solvers may apply their caches, e.g. CoinModel.writeMps is not const.
            virtual void print_mps_file         (const std::string& p_path)   = 0;

            virtual ~ILPSolverInterface() noexcept {}
    };
}

#endif
