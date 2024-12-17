#include "ilp_solver_collect.hpp"

#include "utility.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <cassert>
#include <format>
#include <span>
#include <sstream>
#include <vector>


namespace ilp_solver
{

int ILPSolverCollect::get_num_constraints() const
{
    return isize(d_ilp_data.constraint_lower);
}


int ILPSolverCollect::get_num_variables() const
{
    return isize(d_ilp_data.variable_lower);
}


namespace
{
    // Return string prefixed with p_type, followed by p_num left aligned and padded to 10 chars with spaces.
    std::string to_name(int p_num, char p_type)
    {
        return std::format("{}{:<10}", p_type, p_num);
    }


    std::vector<std::string> handle_mps_rows(const ILPData& p_data)
    {
        std::vector<std::string> cons_names;
        cons_names.reserve(p_data.constraint_lower.size() + 2);

        int eq_cons{0};
        int leq_cons{0};
        int geq_cons{0};
        int range_cons{0};

        std::stringstream cons, rhs, rhs_range;

        for (int i = 0; i < isize(p_data.constraint_lower); ++i)
        {
            const auto lower = p_data.constraint_lower[i];
            const auto upper = p_data.constraint_upper[i];
            if (lower == upper)
            {
                cons_names.emplace_back(to_name(eq_cons++, 'E'));
                cons << " E  " << cons_names.back() << '\n';
                rhs << "    RHS             " << cons_names.back() << ' ' << lower << '\n';
            }
            else
            {
                if (lower >= c_neg_inf_bound)
                {
                    if (upper <= c_pos_inf_bound)
                    {
                        cons_names.emplace_back(to_name(range_cons++, 'R'));
                        cons << " E  " << cons_names.back() << '\n'; // Ranges are handled differently.
                        rhs << "    RHS             " << cons_names.back() << ' ' << lower << '\n';
                        rhs_range << "    RHS             " << cons_names.back() << ' ' << upper - lower << '\n';
                    }
                    else
                    {
                        cons_names.emplace_back(to_name(geq_cons++, 'G'));
                        cons << " G  " << cons_names.back() << '\n';
                        rhs << "    RHS             " << cons_names.back() << ' ' << lower << '\n';
                    }
                }
                else
                {
                    if (upper <= c_pos_inf_bound)
                    {
                        cons_names.emplace_back(to_name(leq_cons++, 'L'));
                        cons << " L  " << cons_names.back() << '\n';
                        rhs << "    RHS             " << cons_names.back() << ' ' << upper << '\n';
                    }
                    else
                    {
                        cons_names.emplace_back("");
                    }
                }
            }
        }
        cons_names.emplace_back("ROWS\n N  OBJ\n" + std::move(cons).str());
        cons_names.emplace_back("RHS\n" + std::move(rhs).str());
        auto ranges = std::move(rhs_range).str();
        if (!ranges.empty())
            cons_names.back().append("RANGES\n" + ranges);

        return cons_names;
    }

    std::string handle_mps_cols(const ILPData& p_data, std::span<std::string> p_names, boost::filesystem::ofstream& v_outstream)
    {
        std::stringstream bounds;

        v_outstream << "COLUMNS\n";
        for (int i = 0; i < isize(p_data.objective); ++i)
        {
            const auto name = to_name(i, 'X');
            const auto obj  = p_data.objective[i];
            const auto ub   = p_data.variable_upper[i];
            const auto lb   = p_data.variable_lower[i];
            const auto type = p_data.variable_type[i];

            std::string bound1;
            std::string bound2;

            if (type == VariableType::BINARY)
                bounds << " BV BOUND           " << name << " \n";
            else if (type == VariableType::INTEGER)
            {
                bounds << " UI BOUND           " << name << ' ' << ub << '\n';
                bounds << " LI BOUND           " << name << ' ' << lb << '\n';
            }
            else
            {
                bounds << " UP BOUND           " << name << ' ' << ub << '\n';
                bounds << " LO BOUND           " << name << ' ' << lb << '\n';
            }

            v_outstream << "    " << name << ' ' << "OBJ             " << obj << '\n';

            for (int j = 0; j < isize(p_data.matrix.d_indices); ++j)
            {
                double value = 0.;
                for (int k = 0; k < isize(p_data.matrix.d_indices[j]); ++k)
                {
                    if (p_data.matrix.d_indices[j][k] == i)
                    {
                        value = p_data.matrix.d_values[j][k];
                        break;
                    }
                }

                v_outstream << "    " << name << ' ' << p_names[j] << ' ' << value << '\n';
            }
        }
        return "BOUNDS\n" + std::move(bounds).str();
    }
} // namespace

void ILPSolverCollect::print_mps_file(const std::string& p_filename)
{
    boost::filesystem::ofstream outstream{boost::filesystem::path(p_filename)};
    assert(outstream);
    assert(d_ilp_data.constraint_lower.size() == d_ilp_data.constraint_upper.size());

    auto names = handle_mps_rows(d_ilp_data);

    outstream << "NAME\n";
    // Print Rows.
    outstream << names[names.size() - 2];

    // Prints columns;
    auto bounds = handle_mps_cols(d_ilp_data, names, outstream);

    // Prints RHS and RANGE
    outstream << names.back();

    // Prints Bounds
    outstream << bounds;

    // Finished.
    outstream << "ENDATA\n";
}


ILPSolverCollect::ILPSolverCollect()
{
    set_default_parameters(this);
}


void ILPSolverCollect::add_variable_impl(VariableType p_type, double p_objective, double p_lower_bound,
                                         double             p_upper_bound, const std::string& /* p_name */,
                                         OptionalValueArray p_row_values, OptionalIndexArray p_row_indices)
{
    if (p_row_values)
    {
        if (!p_row_indices)
        {
            assert(p_row_values->size() == d_ilp_data.matrix.d_values.size());
            d_ilp_data.matrix.append_column(*p_row_values);
        }
        else
        {
            assert(p_row_values->size() == p_row_indices->size());
            assert(p_row_indices->size() <= d_ilp_data.matrix.d_values.size());
            d_ilp_data.matrix.append_column(*p_row_indices, *p_row_values);
        }
    }
    else
    {
        assert(!p_row_indices);
        d_ilp_data.matrix.append_column(IndexArray(), ValueArray());
    }

    d_ilp_data.objective.push_back(p_objective);
    d_ilp_data.variable_lower.push_back(p_lower_bound);
    d_ilp_data.variable_upper.push_back(p_upper_bound);
    d_ilp_data.variable_type.push_back(p_type);
}


void ILPSolverCollect::add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                           const std::string& /* p_name */, OptionalIndexArray p_col_indices)
{
    if (!p_col_indices)
    {
        assert(p_col_values.size() == d_ilp_data.objective.size());
        d_ilp_data.matrix.append_row(p_col_values);
    }
    else
    {
        assert(p_col_values.size() == p_col_indices->size());
        assert(p_col_indices->size() <= d_ilp_data.objective.size());
        d_ilp_data.matrix.append_row(*p_col_indices, p_col_values);
    }

    d_ilp_data.constraint_lower.push_back(p_lower_bound);
    d_ilp_data.constraint_upper.push_back(p_upper_bound);
}


void ILPSolverCollect::set_objective_sense_impl(ObjectiveSense p_sense)
{
    d_ilp_data.objective_sense = p_sense;
}


void ILPSolverCollect::set_start_solution(ValueArray p_solution)
{
    d_ilp_data.start_solution.assign(p_solution.begin(), p_solution.end());
}


void ILPSolverCollect::set_num_threads(int p_num_threads)
{
    d_ilp_data.num_threads = p_num_threads;
}


void ILPSolverCollect::set_deterministic_mode(bool p_deterministic)
{
    d_ilp_data.deterministic = p_deterministic;
}


void ILPSolverCollect::set_log_level(int p_level)
{
    d_ilp_data.log_level = p_level;
}


void ILPSolverCollect::set_presolve(bool p_presolve)
{
    d_ilp_data.presolve = p_presolve;
}


void ILPSolverCollect::set_max_seconds_impl(double p_seconds)
{
    d_ilp_data.max_seconds = p_seconds;
}


void ILPSolverCollect::set_max_nodes(int p_nodes)
{
    d_ilp_data.max_nodes = p_nodes;
}


void ILPSolverCollect::set_max_solutions(int p_solutions)
{
    d_ilp_data.max_solutions = p_solutions;
}


void ILPSolverCollect::set_max_abs_gap(double p_abs_gap)
{
    d_ilp_data.max_abs_gap = p_abs_gap;
}


void ILPSolverCollect::set_max_rel_gap(double p_rel_gap)
{
    d_ilp_data.max_rel_gap = p_rel_gap;
}


void ILPSolverCollect::set_cutoff(double p_cutoff)
{
    d_ilp_data.cutoff = p_cutoff;
}

} // namespace ilp_solver
