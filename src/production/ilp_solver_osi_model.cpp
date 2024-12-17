#ifdef WITH_OSI

// Link with the CoinUtils and Osi Libraries.
#pragma comment(lib, "libCoinUtils.lib")
#pragma comment(lib, "libOsi.lib")
#pragma comment(lib, "libOsiClp.lib")

#include "ilp_solver_osi_model.hpp"
#include "utility.hpp"

#include <OsiSolverInterface.hpp>


namespace ilp_solver
{
    int ILPSolverOsiModel::get_num_constraints() const
    {
        return d_cache.numberRows();
    }


    int ILPSolverOsiModel::get_num_variables() const
    {
        return d_cache.numberColumns();
    }


    void ILPSolverOsiModel::print_mps_file(const std::string& p_filename)
    {
        // path,
        // compression (off),
        // format (extra precision),
        // number of values per dataline (1 or 2),
        // keepStrings (no idea what it does, false is default).
        if( d_cache.writeMps(p_filename.c_str(), 0, 1, 1, false) )
            throw std::runtime_error("Could not write mps file.");
    }


    void ILPSolverOsiModel::prepare_impl()
    {
        auto* solver{ get_solver_osi_model() };
        if (d_cache_changed && (d_cache.numberColumns() > 0 || d_cache.numberRows() > 0))
        {
            solver->loadFromCoinModel(d_cache, true);
            d_cache_changed = false;
        }
    }


    void ILPSolverOsiModel::add_variable_impl (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                                               const std::string& p_name, OptionalValueArray p_row_values, OptionalIndexArray p_row_indices)
    {
        // Spaces are problematic when printing to mps.
        auto        name     = replace_spaces(p_name);
        const char* name_ptr = name.empty() ? nullptr : name.c_str();
        // OSI has no special case for binary variables.
        const bool is_integer_or_binary = (p_type != VariableType::CONTINUOUS);

        if (!p_row_values)
        {
            d_cache.addCol(0, nullptr, nullptr, p_lower_bound, p_upper_bound, p_objective, name_ptr, is_integer_or_binary);
        }
        else if (p_row_indices)
        {
            d_cache.addCol(isize(*p_row_values), p_row_indices->data(), p_row_values->data(),
                           p_lower_bound, p_upper_bound, p_objective, name_ptr, is_integer_or_binary);
        }
        else
        {
            d_sparse.init_from_dense(*p_row_values);
            d_cache.addCol(isize(d_sparse), d_sparse.indices().data(), d_sparse.values().data(),
                           p_lower_bound, p_upper_bound, p_objective, name_ptr, is_integer_or_binary);
        }
        d_cache_changed = true;
    }


    void ILPSolverOsiModel::add_constraint_impl (double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                                 const std::string& p_name, OptionalIndexArray p_col_indices)
    {
        // Spaces are problematic when printing to mps.
        auto        name     = replace_spaces(p_name);
        const char* name_ptr = name.empty() ? nullptr : name.c_str();

        if (p_col_indices)
        {
            d_cache.addRow(isize(p_col_values), p_col_indices->data(), p_col_values.data(),
                           p_lower_bound, p_upper_bound, name_ptr);
        }
        else
        {
            d_sparse.init_from_dense(p_col_values);
            d_cache.addRow(isize(d_sparse), d_sparse.indices().data(), d_sparse.values().data(),
                           p_lower_bound, p_upper_bound, name_ptr);
        }
        d_cache_changed = true;
    }
}

#endif
