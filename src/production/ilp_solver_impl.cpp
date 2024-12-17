#include "ilp_solver_impl.hpp"

#include <cassert>
#include <limits>

using std::string;
using std::vector;

namespace ilp_solver
{
    void set_default_parameters(ILPSolverInterface* p_solver)
    {
        p_solver->set_num_threads       (c_default_num_threads);
        p_solver->set_deterministic_mode(c_default_deterministic);
        p_solver->set_log_level         (c_default_log_level);
        p_solver->set_presolve          (c_default_presolve);

        p_solver->set_max_seconds       (c_default_max_seconds);
        p_solver->set_max_nodes         (c_default_max_nodes);
        p_solver->set_max_solutions     (c_default_max_solutions);
        p_solver->set_max_abs_gap       (c_default_max_abs_gap);
        p_solver->set_max_rel_gap       (c_default_max_rel_gap);
    }


    void ILPSolverImpl::add_variable_boolean(double p_objective, const string& p_name)
    {
        add_variable_impl (VariableType::BINARY, p_objective, 0., 1., p_name);
    }


    void ILPSolverImpl::add_variable_boolean(const vector<double>& p_row_values, double p_objective, const string& p_name)
    {
        add_variable_impl (VariableType::BINARY, p_objective, 0., 1., p_name, &p_row_values);
    }


    void ILPSolverImpl::add_variable_boolean(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, const string& p_name)
    {
        assert(p_row_values.size() == p_row_indices.size());
        add_variable_impl (VariableType::BINARY, p_objective, 0., 1., p_name, &p_row_values, &p_row_indices);
    }


    void ILPSolverImpl::add_variable_integer(double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_impl (VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name);
    }


    void ILPSolverImpl::add_variable_integer(const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_impl (VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name, &p_row_values);
    }


    void ILPSolverImpl::add_variable_integer(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        assert(p_row_values.size() == p_row_indices.size());
        add_variable_impl (VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name, &p_row_values, &p_row_indices);
    }


    void ILPSolverImpl::add_variable_continuous(double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_impl (VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name);
    }


    void ILPSolverImpl::add_variable_continuous(const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        add_variable_impl (VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name, &p_row_values);
    }


    void ILPSolverImpl::add_variable_continuous(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        assert(p_row_values.size() == p_row_indices.size());
        add_variable_impl (VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name, &p_row_values, &p_row_indices);
    }


    void ILPSolverImpl::add_constraint(const vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        if ( p_upper_bound > c_pos_inf_bound && p_lower_bound < c_neg_inf_bound ) return;
        add_constraint_impl (p_lower_bound, p_upper_bound, p_col_values, p_name);
    }


    void ILPSolverImpl::add_constraint(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const string& p_name)
    {
        if ( p_upper_bound > c_pos_inf_bound && p_lower_bound < c_neg_inf_bound ) return;
        add_constraint_impl (p_lower_bound, p_upper_bound, p_col_values, p_name, &p_col_indices);
    }


    void ILPSolverImpl::add_constraint_upper(const vector<double>& p_col_values, double p_upper_bound, const string& p_name)
    {
        if (p_upper_bound > c_pos_inf_bound) return;
        add_constraint_impl (c_neg_inf, p_upper_bound, p_col_values, p_name);
    }


    void ILPSolverImpl::add_constraint_upper(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_upper_bound, const string& p_name)
    {
        if (p_upper_bound > c_pos_inf_bound) return;
        add_constraint_impl (c_neg_inf, p_upper_bound, p_col_values, p_name, &p_col_indices);
    }


    void ILPSolverImpl::add_constraint_lower(const vector<double>& p_col_values, double p_lower_bound, const string& p_name)
    {
        if (p_lower_bound < c_neg_inf_bound) return;
        add_constraint_impl (p_lower_bound, c_pos_inf, p_col_values, p_name);
    }


    void ILPSolverImpl::add_constraint_lower(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_lower_bound, const string& p_name)
    {
        if (p_lower_bound < c_neg_inf_bound) return;
        add_constraint_impl (p_lower_bound, c_pos_inf, p_col_values, p_name, &p_col_indices);
    }


    void ILPSolverImpl::add_constraint_equality(const vector<double>& p_col_values, double p_value, const string& p_name)
    {
        add_constraint_impl (p_value, p_value, p_col_values, p_name);
    }


    void ILPSolverImpl::add_constraint_equality(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_value, const string& p_name)
    {
        add_constraint_impl (p_value, p_value, p_col_values, p_name, &p_col_indices);
    }


    void ILPSolverImpl::prepare_impl()
    { }


    void ILPSolverImpl::minimize()
    {
        prepare_impl();
        set_objective_sense_impl(ObjectiveSense::MINIMIZE);
        solve_impl();
    }


    void ILPSolverImpl::maximize()
    {
        prepare_impl();
        set_objective_sense_impl(ObjectiveSense::MAXIMIZE);
        solve_impl();
    }
}
