#include "ilp_solver_impl.hpp"

#include "utility.hpp"

#include <algorithm>
#include <cassert>


namespace ilp_solver
{

void set_default_parameters(ILPSolverInterface* p_solver)
{
    p_solver->set_num_threads(c_default_num_threads);
    p_solver->set_deterministic_mode(c_default_deterministic);
    p_solver->set_log_level(c_default_log_level);
    p_solver->set_presolve(c_default_presolve);

    p_solver->set_max_seconds(c_default_max_seconds);
    p_solver->set_max_nodes(c_default_max_nodes);
    p_solver->set_max_solutions(c_default_max_solutions);
    p_solver->set_max_abs_gap(c_default_max_abs_gap);
    p_solver->set_max_rel_gap(c_default_max_rel_gap);
    p_solver->set_cutoff(c_default_cutoff);
}


std::string replace_spaces(const std::string& p_name)
{
    auto name = p_name;
    std::ranges::replace(name, ' ', '_');
    return name;
}


void SparseVec::init_from_dense(ValueArray p_dense_values)
{
    d_indices.clear();
    d_values.clear();
    for (int index = 0; index < isize(p_dense_values); ++index)
    {
        if (auto value = p_dense_values[index]; value != 0.)
        {
            d_indices.push_back(index);
            d_values.push_back(value);
        }
    }
}


void ILPSolverImpl::add_variable_boolean(double p_objective, const std::string& p_name)
{
    add_variable_impl(VariableType::BINARY, p_objective, 0., 1., p_name);
}


void ILPSolverImpl::add_variable_boolean(ValueArray p_row_values, double p_objective, const std::string& p_name)
{
    add_variable_impl(VariableType::BINARY, p_objective, 0., 1., p_name, p_row_values);
}


void ILPSolverImpl::add_variable_boolean(IndexArray p_row_indices, ValueArray p_row_values, double p_objective,
                                         const std::string& p_name)
{
    assert(p_row_values.size() == p_row_indices.size());
    add_variable_impl(VariableType::BINARY, p_objective, 0., 1., p_name, p_row_values, p_row_indices);
}


void ILPSolverImpl::add_variable_integer(double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name)
{
    add_variable_impl(VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name);
}


void ILPSolverImpl::add_variable_integer(ValueArray p_row_values, double p_objective, double p_lower_bound,
                                         double p_upper_bound, const std::string& p_name)
{
    add_variable_impl(VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name, p_row_values);
}


void ILPSolverImpl::add_variable_integer(IndexArray p_row_indices, ValueArray p_row_values, double p_objective,
                                         double p_lower_bound, double p_upper_bound, const std::string& p_name)
{
    assert(p_row_values.size() == p_row_indices.size());
    add_variable_impl(VariableType::INTEGER, p_objective, p_lower_bound, p_upper_bound, p_name, p_row_values, p_row_indices);
}


void ILPSolverImpl::add_variable_continuous(double p_objective, double p_lower_bound, double p_upper_bound,
                                            const std::string& p_name)
{
    add_variable_impl(VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name);
}


void ILPSolverImpl::add_variable_continuous(ValueArray p_row_values, double p_objective, double p_lower_bound,
                                            double p_upper_bound, const std::string& p_name)
{
    add_variable_impl(VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name, p_row_values);
}


void ILPSolverImpl::add_variable_continuous(IndexArray p_row_indices, ValueArray p_row_values, double p_objective,
                                            double p_lower_bound, double p_upper_bound, const std::string& p_name)
{
    assert(p_row_values.size() == p_row_indices.size());
    add_variable_impl(VariableType::CONTINUOUS, p_objective, p_lower_bound, p_upper_bound, p_name, p_row_values, p_row_indices);
}


void ILPSolverImpl::add_constraint(ValueArray p_col_values, double p_lower_bound, double p_upper_bound, const std::string& p_name)
{
    if (p_upper_bound > c_pos_inf_bound && p_lower_bound < c_neg_inf_bound)
        return;
    add_constraint_impl(p_lower_bound, p_upper_bound, p_col_values, p_name);
}


void ILPSolverImpl::add_constraint(IndexArray p_col_indices, ValueArray p_col_values, double p_lower_bound,
                                   double p_upper_bound, const std::string& p_name)
{
    if (p_upper_bound > c_pos_inf_bound && p_lower_bound < c_neg_inf_bound)
        return;
    add_constraint_impl(p_lower_bound, p_upper_bound, p_col_values, p_name, p_col_indices);
}


void ILPSolverImpl::add_constraint_upper(ValueArray p_col_values, double p_upper_bound, const std::string& p_name)
{
    if (p_upper_bound > c_pos_inf_bound)
        return;
    add_constraint_impl(c_neg_inf, p_upper_bound, p_col_values, p_name);
}


void ILPSolverImpl::add_constraint_upper(IndexArray p_col_indices, ValueArray p_col_values, double p_upper_bound,
                                         const std::string& p_name)
{
    if (p_upper_bound > c_pos_inf_bound)
        return;
    add_constraint_impl(c_neg_inf, p_upper_bound, p_col_values, p_name, p_col_indices);
}


void ILPSolverImpl::add_constraint_lower(ValueArray p_col_values, double p_lower_bound, const std::string& p_name)
{
    if (p_lower_bound < c_neg_inf_bound)
        return;
    add_constraint_impl(p_lower_bound, c_pos_inf, p_col_values, p_name);
}


void ILPSolverImpl::add_constraint_lower(IndexArray p_col_indices, ValueArray p_col_values, double p_lower_bound,
                                         const std::string& p_name)
{
    if (p_lower_bound < c_neg_inf_bound)
        return;
    add_constraint_impl(p_lower_bound, c_pos_inf, p_col_values, p_name, p_col_indices);
}


void ILPSolverImpl::add_constraint_equality(ValueArray p_col_values, double p_value, const std::string& p_name)
{
    add_constraint_impl(p_value, p_value, p_col_values, p_name);
}


void ILPSolverImpl::add_constraint_equality(IndexArray p_col_indices, ValueArray p_col_values, double p_value,
                                            const std::string& p_name)
{
    add_constraint_impl(p_value, p_value, p_col_values, p_name, p_col_indices);
}


void ILPSolverImpl::set_max_seconds(double p_seconds)
{
    d_max_seconds = p_seconds;
    set_max_seconds_impl(p_seconds);
}


void ILPSolverImpl::prepare_impl()
{}


void ILPSolverImpl::minimize()
{
    if (d_max_seconds <= 0)
        return;

    prepare_impl();
    set_objective_sense_impl(ObjectiveSense::MINIMIZE);
    solve_impl();
}


void ILPSolverImpl::maximize()
{
    if (d_max_seconds <= 0)
        return;

    prepare_impl();
    set_objective_sense_impl(ObjectiveSense::MAXIMIZE);
    solve_impl();
}

} // namespace ilp_solver
