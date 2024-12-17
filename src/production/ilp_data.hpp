#pragma once

#include "ilp_solver_impl.hpp"
#include "ilp_solver_interface.hpp"
#include "utility.hpp"

#include <algorithm>
#include <limits>
#include <span>
#include <vector>

namespace ilp_solver
{
// Shared POD base of ILPData and ILPDataView.
struct ILPDataBase
{
    ObjectiveSense objective_sense{ObjectiveSense::MINIMIZE};

    // Defaults will be overwritten in ilp_solver_collect,
    // but are initialized to the same constants to be sure.
    int    num_threads   { c_default_num_threads   };
    bool   deterministic { c_default_deterministic };
    int    log_level     { c_default_log_level     };
    bool   presolve      { c_default_presolve      };
    double max_seconds   { c_default_max_seconds   };
    int    max_nodes     { c_default_max_nodes     };
    int    max_solutions { c_default_max_solutions };
    double max_abs_gap   { c_default_max_abs_gap   };
    double max_rel_gap   { c_default_max_rel_gap   };
    double cutoff        { c_default_cutoff        };

    virtual ~ILPDataBase() = default;
};


struct ILPData final : public ILPDataBase
{
    // Internally, a vector of rows/constraints is stored. The size of the outer vector is the number of constraints.
    // The inner vectors store only non-zero entries (sparse matrix).
    // Number of variables = number of columns = d_num_cols.
    // If no constraints are given, we can not know the number of variables. (m x 0 can be stored, 0 x n can not).
    // Appending a row is faster than appending a column.
    struct Matrix
    {
        void append_column(ValueArray p_row_values)
        {
            assert(p_row_values.size() == d_values.size());

            if (d_values.empty())
                return;

            // Set specified values.
            for (int i = 0; i < isize(d_values); ++i)
            {
                if (p_row_values[i] != 0.)
                {
                    d_values[i].push_back(p_row_values[i]);
                    d_indices[i].push_back(d_num_cols);
                }
            }
            ++d_num_cols;
        }


        void append_column(IndexArray p_row_indices, ValueArray p_row_values)
        {
            assert(p_row_indices.size() == p_row_values.size());

            if (d_values.empty())
                return;

            // Set specified values.
            for (auto i = 0; i < isize(p_row_indices); ++i)
            {
                const auto row_index = p_row_indices[i];
                const auto value     = p_row_values[i];
                assert(0 <= row_index && row_index < isize(d_values));

                if (value != 0.)
                {
                    d_values[row_index].push_back(value);
                    d_indices[row_index].push_back(d_num_cols);
                }
            }
            ++d_num_cols;
        }


        void append_row(ValueArray p_col_values)
        {
            // Count non-zero values.
            int num_non_zero_values = 0;
            for (auto value : p_col_values)
                num_non_zero_values += (value != 0);

            // Add new row.
            auto& values = d_values.emplace_back();
            auto& indices = d_indices.emplace_back();

            if (num_non_zero_values == 0)
                return;

            values.reserve(num_non_zero_values);
            indices.reserve(num_non_zero_values);

            // Set specified values.
            for (int i = 0; i < isize(p_col_values); ++i)
            {
                if (p_col_values[i] != 0.)
                {
                    values.push_back(p_col_values[i]);
                    indices.push_back(i);
                }
            }

            d_num_cols = std::max(d_num_cols, isize(p_col_values));
        }


        void append_row(IndexArray p_col_indices, ValueArray p_col_values)
        {
            assert(p_col_indices.size() == p_col_values.size());

            // Add new row.
            auto& values  = d_values.emplace_back();
            auto& indices = d_indices.emplace_back();
            values.reserve(p_col_indices.size());
            indices.reserve(p_col_indices.size());

            // Set specified values.
            for (int i = 0; i < isize(p_col_values); ++i)
            {
                if (p_col_values[i] != 0.)
                {
                    values.push_back(p_col_values[i]);
                    indices.push_back(p_col_indices[i]);
                    d_num_cols = std::max(d_num_cols, p_col_indices[i] + 1);
                }
            }
        }

        std::vector<std::vector<double>> d_values;  // For each row: non-zero values.
        std::vector<std::vector<int>>    d_indices; // For each row: column indices of the non-zero values.
        int                              d_num_cols{0};

    };

    Matrix                    matrix;
    std::vector<double>       objective;
    std::vector<double>       variable_lower;
    std::vector<double>       variable_upper;
    std::vector<double>       constraint_lower;
    std::vector<double>       constraint_upper;
    std::vector<VariableType> variable_type;
    std::vector<double>       start_solution;
};


// Same as ILPData, but inner containers are non-owning.
struct ILPDataView final : public ILPDataBase
{
    struct Matrix
    {
        std::vector<std::span<double>> d_values;
        std::vector<std::span<int>>    d_indices;
        int                            d_num_cols{0};
    };

    Matrix                  matrix;
    std::span<double>       objective;
    std::span<double>       variable_lower;
    std::span<double>       variable_upper;
    std::span<double>       constraint_lower;
    std::span<double>       constraint_upper;
    std::span<VariableType> variable_type;
    std::span<double>       start_solution;
};


struct ILPSolutionData
{
    std::vector<double> solution;
    std::vector<double> dual_sol;
    double              objective{std::numeric_limits<double>::quiet_NaN()};
    SolutionStatus      solution_status{SolutionStatus::NO_SOLUTION};
    double              cpu_time_sec{};
    double              peak_memory{};

    ILPSolutionData() = default;

    explicit ILPSolutionData(ObjectiveSense p_objective_sense)
        : objective(p_objective_sense == ObjectiveSense::MINIMIZE ? std::numeric_limits<double>::max()
                                                                  : std::numeric_limits<double>::lowest()),
          solution_status(SolutionStatus::NO_SOLUTION)
        {}
};
}
