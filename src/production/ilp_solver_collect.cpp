#include "ilp_solver_collect.hpp"

#include <cassert>

using std::string;
using std::vector;

typedef vector< vector<double> > Matrix;

namespace ilp_solver
{
    static void append_column(Matrix* v_matrix, const vector<int>& p_row_indices, const vector<double>& p_row_values)
    {
        if (v_matrix->empty())
            return;

        // enlarge matrix
        for (auto& row: *v_matrix)
            row.push_back(0.0);

        // set specified values
        for (auto i = 0; i < (int) p_row_indices.size(); ++i)
        {
            const auto row_index = p_row_indices[i];
            const auto value = p_row_values[i];
            (*v_matrix)[row_index].back() = value;
        }
    }


    static void append_row(Matrix* v_matrix, int p_num_cols, const vector<int>& p_col_indices, const vector<double>& p_col_values)
    {
        // enlarge matrix
        v_matrix->emplace_back(p_num_cols, 0.0);

        // set specified values
        auto& row = v_matrix->back();
        for (auto i = 0; i < (int) p_col_indices.size(); ++i)
        {
            const auto col_index = p_col_indices[i];
            const auto value = p_col_values[i];
            row[col_index] = value;
        }
    }


    void ILPSolverCollect::do_add_variable(const vector<int>& p_row_indices, const vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const string& /* p_name */, VariableType p_type)
    {
        assert(p_row_indices.size() == p_row_values.size());

        append_column(&d_ilp_data.matrix, p_row_indices, p_row_values);
        d_ilp_data.objective.push_back(p_objective);
        d_ilp_data.variable_lower.push_back(p_lower_bound);
        d_ilp_data.variable_upper.push_back(p_upper_bound);
        d_ilp_data.variable_type.push_back(p_type);
    }


    void ILPSolverCollect::do_add_constraint(const vector<int>& p_col_indices, const vector<double>& p_col_values, double p_lower_bound, double p_upper_bound, const string& /* p_name */)
    {
        assert(p_col_indices.size() == p_col_values.size());

        const auto num_cols = (int) d_ilp_data.objective.size();

        append_row(&d_ilp_data.matrix, num_cols, p_col_indices, p_col_values);
        d_ilp_data.constraint_lower.push_back(p_lower_bound);
        d_ilp_data.constraint_upper.push_back(p_upper_bound);
    }


    void ILPSolverCollect::do_set_objective_sense(ObjectiveSense p_sense)
    {
        d_ilp_data.objective_sense = p_sense;
    }


    void ILPSolverCollect::do_prepare_and_solve(const std::vector<double>& p_start_solution,
                                                int p_num_threads, bool p_deterministic, int p_log_level, double p_max_seconds)
    {
        d_ilp_data.start_solution = p_start_solution;

        d_ilp_data.num_threads   = p_num_threads;
        d_ilp_data.deterministic = p_deterministic;
        d_ilp_data.log_level     = p_log_level;
        d_ilp_data.max_seconds   = p_max_seconds;

        do_solve(d_ilp_data);
    }
}
