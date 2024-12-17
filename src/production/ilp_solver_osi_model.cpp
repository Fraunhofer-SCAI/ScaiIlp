#if WITH_OSI == 1

// Link with the CoinUtils and Osi Libraries.
#pragma comment(lib, "libCoinUtils.lib")
#pragma comment(lib, "libOsi.lib")
#pragma comment(lib, "libOsiClp.lib")

#include "ilp_solver_osi_model.hpp"

#pragma warning(push)
#pragma warning(disable : 4309) // silence warning in CBC concerning truncations of constant values in 64 bit.
#include "OsiSolverInterface.hpp"
#pragma warning(pop)

#include <optional>
#include <cassert>
#include <algorithm>
#include <numeric>

using std::string;
using std::vector;

// States whether consecutive elements of each column are contiguous in memory.
// (If not, consecutive elements of each row are contiguous in memory.)
constexpr auto c_column_ordered = false;

constexpr auto c_test_for_duplicate_index = false;

namespace ilp_solver
{
    namespace
    {

        // Prune zeros before constructing a coin-packed vector.
        // Takes pointers to the vectors or nullptrs as parameters.
        // If p_values is a nullptr, does nothing and returns 0, nullptr and nullptr.
        // If p_values is valid,
        //     prunes all zeros from it and constructs a new value array if there were any,
        //     and a new index array if there are any zeros or if p_indices == nullptr.
        class ZeroPruner
        {
        public:
            ZeroPruner (const std::vector<int>* p_indices, const std::vector<double>* p_values);
            ~ZeroPruner();

            int           size()    const { return d_num_indices; }
            const double* values()  const { return d_values; }
            const int*    indices() const { return d_indices; }
        private:
            static constexpr int c_owns_values{2};
            static constexpr int c_owns_indices{1};

            double* d_values      {nullptr};
            int*    d_indices     {nullptr};
            int     d_num_indices {0};
            int     d_owns        {0};
        };

        ZeroPruner::ZeroPruner(const std::vector<int>* p_indices, const std::vector<double>* p_values)
        {
            if (p_values != nullptr) {
                assert ((p_indices == nullptr) || (p_indices->size() == p_values->size()));

                int num_zeros{ static_cast<int>(std::count(p_values->begin(), p_values->end(), 0.)) };

                if (num_zeros > 0)
                {
                    d_num_indices = static_cast<int>(p_values->size()) - num_zeros;
                    d_owns        = c_owns_values | c_owns_indices;
                    d_values      = new double[d_num_indices];
                    d_indices     = new int   [d_num_indices];

                    for (int i = 0, j = 0; i < static_cast<int>(p_values->size()); i++)
                    {
                        auto value = (*p_values)[i];
                        // Construct the new arrays. If we have no indices, use the current index.
                        if (value != 0.)
                        {
                            d_values [j]   = value;
                            d_indices[j++] = (p_indices != nullptr) ? (*p_indices)[i] : i;
                        }
                    }
                }
                else
                {
                    // const_cast is valid since we do not ever manipulate this data
                    // and the getter-methods return const pointers again.
                    d_values      = const_cast<double*>(p_values->data());
                    d_num_indices = static_cast<int>(p_values->size());

                    if (p_indices == nullptr)
                    {
                        d_owns    = c_owns_indices;
                        d_indices = new int[d_num_indices];
                        std::iota(d_indices, d_indices + d_num_indices, 0);
                    }
                    else
                    {
                        d_owns    = 0;
                        d_indices = const_cast<int*>(p_indices->data());
                    }
                }
            }
        }

        ZeroPruner::~ZeroPruner() noexcept
        {
            if (d_owns & c_owns_values)  delete[] d_values;
            if (d_owns & c_owns_indices) delete[] d_indices;
        }
    }


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
            throw std::exception("Could not write mps file.");
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
                                               const std::string& p_name, const std::vector<double>* p_row_values, const std::vector<int>* p_row_indices)
    {
        ZeroPruner pruner{p_row_indices, p_row_values};
        assert (pruner.size() <= get_num_constraints());

        // OSI has no special case for binary variables.
        bool is_integer_or_binary{ (p_type == VariableType::CONTINUOUS) ? false : true };
        if (!p_name.empty())
        {
            // Spaces are problematic when printing to mps.
            auto name = p_name;
            std::replace(name.begin(), name.end(), ' ', '_');
            d_cache.addCol(pruner.size(), pruner.indices(), pruner.values(), p_lower_bound, p_upper_bound, p_objective, name.c_str(), is_integer_or_binary);
        }
        else
            d_cache.addCol(pruner.size(), pruner.indices(), pruner.values(), p_lower_bound, p_upper_bound, p_objective, nullptr, is_integer_or_binary);
        d_cache_changed = true;
    }


    void ILPSolverOsiModel::add_constraint_impl (double p_lower_bound, double p_upper_bound, const std::vector<double>& p_col_values,
                                                 const std::string& p_name, const std::vector<int>* p_col_indices)
    {
        ZeroPruner pruner{p_col_indices, &p_col_values};

        if (!p_name.empty())
        {
            // Spaces are problematic when printing to mps.
            auto name = p_name;
            std::replace(name.begin(), name.end(), ' ', '_');
            d_cache.addRow(pruner.size(), pruner.indices(), pruner.values(), p_lower_bound, p_upper_bound, name.c_str());
        }
        else
            d_cache.addRow(pruner.size(), pruner.indices(), pruner.values(), p_lower_bound, p_upper_bound);
        d_cache_changed = true;
    }
}

#endif
