#include "ilp_solver_interface_impl.hpp"

#include <cassert>
#include <limits>

using std::string;
using std::vector;

namespace ilp_solver
{
    ILPSolverInterfaceImpl::ILPSolverInterfaceImpl()
        : d_num_threads(1), d_deterministic(true), d_log_level(0), d_max_seconds(std::numeric_limits<double>::max())
        {}

    void ILPSolverInterfaceImpl::add_variable_boolean(double p_objective, const string& p_name)
    {
        add_variable_boolean(vector<int>(), vector<double>(), p_objective, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_boolean(const vector<double>& p_row_values, double p_objective, const string& p_name)
    {
        assert(d_all_row_indices.size() == p_row_values.size());
        add_variable_boolean(d_all_row_indices, p_row_values, p_objective, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_boolean(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, const string& p_name)
    {
        add_variable_and_update_index_vector(p_row_indices, p_row_values, p_objective, 0, 1, p_name, VariableType::INTEGER);
    }

    void ILPSolverInterfaceImpl::add_variable_integer(double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_integer(vector<int>(), vector<double>(), p_objective, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_integer(const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        assert(d_all_row_indices.size() == p_row_values.size());
        add_variable_integer(d_all_row_indices, p_row_values, p_objective, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_integer(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_and_update_index_vector(p_row_indices, p_row_values, p_objective, p_lower_bound, p_upper_bound, p_name, VariableType::INTEGER);
    }

    void ILPSolverInterfaceImpl::add_variable_continuous(double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_continuous(vector<int>(), vector<double>(), p_objective, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_continuous(const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        assert(d_all_row_indices.size() == p_row_values.size());
        add_variable_continuous(d_all_row_indices, p_row_values, p_objective, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_variable_continuous(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_and_update_index_vector(p_row_indices, p_row_values, p_objective, p_lower_bound, p_upper_bound, p_name, VariableType::CONTINUOUS);
    }

    void ILPSolverInterfaceImpl::add_constraint(const vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        assert(d_all_col_indices.size() == p_col_values.size());
        add_constraint(d_all_col_indices, p_col_values, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        if ((p_lower_bound <= 0.5*std::numeric_limits<double>::lowest()) && (p_upper_bound >= 0.5*std::numeric_limits<double>::max()))    // no restriction
            return;
        add_constraint_and_update_index_vector(p_col_indices, p_col_values, p_lower_bound, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_upper(const vector<double>& p_col_values, double p_upper_bound, const string& p_name)
    {
        assert(d_all_col_indices.size() == p_col_values.size());
        add_constraint_upper(d_all_col_indices, p_col_values, p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_upper(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_upper_bound, const string& p_name)
    {
        if (p_upper_bound >= 0.5*std::numeric_limits<double>::max())    // no restriction
            return;
        add_constraint_and_update_index_vector(p_col_indices, p_col_values, std::numeric_limits<double>::lowest(), p_upper_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_lower(const vector<double>& p_col_values, double p_lower_bound, const string& p_name)
    {
        assert(d_all_col_indices.size() == p_col_values.size());
        add_constraint_lower(d_all_col_indices, p_col_values, p_lower_bound, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_lower(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_lower_bound, const string& p_name)
    {
        if (p_lower_bound <= 0.5*std::numeric_limits<double>::lowest())   // no restriction
            return;
        add_constraint_and_update_index_vector(p_col_indices, p_col_values, p_lower_bound, std::numeric_limits<double>::max(), p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_equality(const vector<double>& p_col_values, double p_value, const string& p_name)
    {
        assert(d_all_col_indices.size() == p_col_values.size());
        add_constraint_equality(d_all_col_indices, p_col_values, p_value, p_name);
    }

    void ILPSolverInterfaceImpl::add_constraint_equality(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_value, const string& p_name)
    {
        add_constraint_and_update_index_vector(p_col_indices, p_col_values, p_value, p_value, p_name);
    }

    void ILPSolverInterfaceImpl::minimize()
    {
        do_set_objective_sense(ObjectiveSense::MINIMIZE);
        do_prepare_and_solve(d_num_threads, d_deterministic, d_log_level, d_max_seconds);
    }

    void ILPSolverInterfaceImpl::maximize()
    {
        do_set_objective_sense(ObjectiveSense::MAXIMIZE);
        do_prepare_and_solve(d_num_threads, d_deterministic, d_log_level, d_max_seconds);
    }

    const vector<double> ILPSolverInterfaceImpl::get_solution() const
    {
        auto solution = do_get_solution();
        if (solution == nullptr)
            return vector<double>();
        else
            return vector<double>(solution, solution + d_all_col_indices.size());
    }

    double ILPSolverInterfaceImpl::get_objective() const
    {
        return do_get_objective();
    }

    SolutionStatus ILPSolverInterfaceImpl::get_status() const
    {
        return do_get_status();
    }

    void ILPSolverInterfaceImpl::set_num_threads(int p_num_threads)
    {
        d_num_threads = p_num_threads;
    }

    void ILPSolverInterfaceImpl::set_deterministic_mode(bool p_deterministic)
    {
        d_deterministic = p_deterministic;
    }

    void ILPSolverInterfaceImpl::set_log_level(int p_level)
    {
        d_log_level = p_level;
    }

    void ILPSolverInterfaceImpl::set_max_seconds(double p_seconds)
    {
        d_max_seconds = p_seconds;
    }

    void ILPSolverInterfaceImpl::add_variable_and_update_index_vector(const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name, VariableType p_type)
    {
        d_all_col_indices.push_back((int) d_all_col_indices.size());    // update d_all_col_indices
        do_add_variable(p_row_indices, p_row_values, p_objective, p_lower_bound, p_upper_bound, p_name, p_type);
    }

    void ILPSolverInterfaceImpl::add_constraint_and_update_index_vector(const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const std::string& p_name)
    {
        d_all_row_indices.push_back((int) d_all_row_indices.size());    // update d_all_row_indices
        do_add_constraint(p_col_indices, p_col_values, p_lower_bound, p_upper_bound, p_name);
    }
}
