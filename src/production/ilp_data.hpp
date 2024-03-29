#pragma once

#include "ilp_solver_impl.hpp"
#include "ilp_solver_interface.hpp"
#include "utility.hpp"

#include <limits>
#include <vector>

namespace ilp_solver
{
    struct ILPData
    {
        // Inner vectors are rows/constraints. The size of the outer vector is the number of constraints.
        // The size of the inner vectors is the number of variables.
        // If no constraints are given, we can not know the number of variables. (m x 0 can be stored, 0 x n can not).
        struct Matrix : public std::vector<std::vector<double>>
        {
            const std::vector<std::vector<double>>& to_base() const noexcept { return *this; }
            std::vector<std::vector<double>>& to_base() noexcept { return *this; }

            void append_column(ValueArray p_row_values)
            {
                // set specified values
                for (int i = 0; i < isize(*this); i++)
                    (*this)[i].push_back(p_row_values[i]);
            }


            void append_column(IndexArray p_row_indices, ValueArray p_row_values)
            {
                if (empty())
                    return;

                // enlarge matrix
                for (auto& row : *this)
                    row.push_back(0.0);

                // set specified values
                for (auto i = 0; i < isize(p_row_indices); ++i)
                {
                    const auto row_index = p_row_indices[i];
                    const auto value     = p_row_values[i];
                    assert(0 <= row_index && row_index < isize(*this));
                    (*this)[row_index].back() = value;
                }
            }


            void append_row(ValueArray p_col_values)
            {
                // enlarge matrix
                emplace_back(p_col_values.begin(), p_col_values.end());
            }


            void append_row(int p_num_cols, IndexArray p_col_indices, ValueArray p_col_values)
            {
                // enlarge matrix
                emplace_back(p_num_cols, 0.0);

                // set specified values
                auto& row = back();
                for (auto i = 0; i < isize(p_col_indices); ++i)
                {
                    const auto col_index = p_col_indices[i];
                    const auto value     = p_col_values[i];
                    assert(0 <= col_index && col_index < isize(row));
                    row[col_index] = value;
                }
            }
        };

        Matrix                    matrix;
        std::vector<double>       objective;
        std::vector<double>       variable_lower;
        std::vector<double>       variable_upper;
        std::vector<double>       constraint_lower;
        std::vector<double>       constraint_upper;
        std::vector<VariableType> variable_type;
        std::vector<double>       start_solution;
        ObjectiveSense            objective_sense{ObjectiveSense::MINIMIZE};

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

        ILPData() = default;
    };


    struct ILPSolutionData
    {
        std::vector<double> solution;
        double objective;
        SolutionStatus solution_status;

        ILPSolutionData()
            : objective(std::numeric_limits<double>::quiet_NaN()),
              solution_status(SolutionStatus::NO_SOLUTION)
            {}

        explicit ILPSolutionData(ObjectiveSense p_objective_sense)
            : objective(p_objective_sense == ObjectiveSense::MINIMIZE ? std::numeric_limits<double>::max()
                                                                      : std::numeric_limits<double>::lowest()),
              solution_status(SolutionStatus::NO_SOLUTION)
            {}
    };
}
