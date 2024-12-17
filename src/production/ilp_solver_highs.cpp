#if defined(WITH_HIGHS) && (_WIN64 == 1)

#include "ilp_solver_highs.hpp"

#include "utility.hpp"

#include <cassert>
#include <format>

// Assert that a call to a HiGHS function did return OK.
// Wrapping this in a function instead of a macro would result in way less readable error messages, sadly.
#ifdef NDEBUG
#define ASSERT_OK(expression) expression
#else
#define ASSERT_OK(expression) assert(expression == HighsStatus::kOk)
#endif

namespace ilp_solver
{

ILPSolverHighs::ILPSolverHighs()
{
    set_default_parameters(this);

    // Disable presolve to enforce the time limit, as presolve does not respect it
    // see https://github.com/ERGO-Code/HiGHS/issues/1278
    set_presolve(false);
    ASSERT_OK(d_highs.setOptionValue("presolve_reduction_limit", 0));
    ASSERT_OK(d_highs.setOptionValue("restart_presolve_reduction_limit", 0));
}


int ILPSolverHighs::get_num_constraints() const
{
    return d_highs.getNumRow();
}


int ILPSolverHighs::get_num_variables() const
{
    return d_highs.getNumCol();
}


std::vector<double> ILPSolverHighs::get_solution() const
{
    auto& solution = d_highs.getSolution();
    if (solution.value_valid)
        return solution.col_value;
    return {};
}


std::vector<double> ILPSolverHighs::get_dual_sol() const
{
    auto& solution = d_highs.getSolution();
    if (solution.value_valid)
        return solution.row_dual;
    return {};
}


double ILPSolverHighs::get_objective() const
{
    return d_highs.getObjectiveValue();
}


SolutionStatus ILPSolverHighs::get_status() const
{
    switch (auto status = d_highs.getModelStatus())
    {
    case HighsModelStatus::kNotset: return SolutionStatus::NO_SOLUTION;
    case HighsModelStatus::kModelEmpty: return SolutionStatus::NO_SOLUTION;
    case HighsModelStatus::kOptimal: return SolutionStatus::PROVEN_OPTIMAL;
    case HighsModelStatus::kInfeasible: return SolutionStatus::PROVEN_INFEASIBLE;
    case HighsModelStatus::kLoadError: [[fallthrough]]; // unused according to comment at enum
    case HighsModelStatus::kModelError: [[fallthrough]];
    case HighsModelStatus::kPresolveError: [[fallthrough]]; // unused according to comment at enum
    case HighsModelStatus::kSolveError: [[fallthrough]];
    case HighsModelStatus::kPostsolveError: [[fallthrough]]; // probably unused according to comment at enum
    // There is a (currently undocumented) option "allow_unbounded_or_infeasible" that defaults to false.
    // So getting kUnboundedOrInfeasible is an error, which is good as there is no corresponding SolutionStatus value.
    case HighsModelStatus::kUnboundedOrInfeasible:
        throw std::runtime_error(std::format("Unexpected HiGHS model status '{}'.", d_highs.modelStatusToString(status)));
    case HighsModelStatus::kUnbounded: return SolutionStatus::PROVEN_UNBOUNDED;
    case HighsModelStatus::kObjectiveBound: break;
    case HighsModelStatus::kObjectiveTarget: break;
    case HighsModelStatus::kTimeLimit: break;
    case HighsModelStatus::kIterationLimit: break;
    case HighsModelStatus::kUnknown: break;
    case HighsModelStatus::kSolutionLimit: break;
    case HighsModelStatus::kInterrupt: break;
    case HighsModelStatus::kMemoryLimit: break;
    }
    // If no solution has been found yet, value_valid is false.
    if (!d_highs.getSolution().value_valid)
        return SolutionStatus::NO_SOLUTION;
    return SolutionStatus::SUBOPTIMAL;
}


void ILPSolverHighs::set_start_solution(ValueArray p_solution)
{
    HighsSolution new_solution{};
    new_solution.col_value.assign(p_solution.begin(), p_solution.end());
    auto set_solution_status = d_highs.setSolution(new_solution);
    // Note that d_highs.getSolution().value_valid will be true for infeasible solutions.
    // Also, setSolution will only reject solutions that have an invalid size.
    // Therefore, we do the additional call to assessPrimalSolution to validate it.
    // It is not documented what exactly the bool parameters represent, but checking them all seems reasonable.
    // Also assessPrimalSolution seems to return kWarning if any of the bools are false.
    bool valid                  = false;
    bool integral               = false;
    bool feasible               = false;
    auto assess_solution_status = d_highs.assessPrimalSolution(valid, integral, feasible);
    if (set_solution_status != HighsStatus::kOk || !d_highs.getSolution().value_valid
        || assess_solution_status != HighsStatus::kOk || !valid || !integral || !feasible)
        throw InvalidStartSolutionException();
}


void ILPSolverHighs::reset_solution()
{
    ASSERT_OK(d_highs.clearSolver());
}


void ILPSolverHighs::set_num_threads(int p_num_threads)
{
    assert(p_num_threads >= 0);     // 0 -> automatic
    // Values >1 might still cause problems, see https://github.com/ERGO-Code/HiGHS/issues/1639
    ASSERT_OK(d_highs.setOptionValue("threads", p_num_threads));
}


void ILPSolverHighs::set_deterministic_mode(bool)
{
    // HiGHS is always deterministic. Therefore, this function is empty.
}


void ILPSolverHighs::set_log_level(int p_level)
{
    ASSERT_OK(d_highs.setOptionValue("log_to_console", p_level != 0));
}


void ILPSolverHighs::set_presolve(bool p_presolve)
{
    // Allowed values are "off", "choose" or "on". Default is "choose".
    ASSERT_OK(d_highs.setOptionValue("presolve", p_presolve ? "choose" : "off"));
}


void ILPSolverHighs::set_max_seconds_impl(double p_seconds)
{
    assert(p_seconds >= 0.);
    ASSERT_OK(d_highs.setOptionValue("time_limit", p_seconds));
}


void ILPSolverHighs::set_max_nodes(int p_nodes)
{
    assert(p_nodes >= 0);
    ASSERT_OK(d_highs.setOptionValue("mip_max_nodes", p_nodes));
}


void ILPSolverHighs::set_max_solutions(int p_solutions)
{
    assert(p_solutions >= 0);
    ASSERT_OK(d_highs.setOptionValue("mip_max_improving_sols", p_solutions));
}


void ILPSolverHighs::set_max_abs_gap(double p_abs_gap)
{
    assert(p_abs_gap >= 0.);
    ASSERT_OK(d_highs.setOptionValue("mip_abs_gap", p_abs_gap));
}


void ILPSolverHighs::set_max_rel_gap(double p_rel_gap)
{
    assert(p_rel_gap >= 0.);
    ASSERT_OK(d_highs.setOptionValue("mip_rel_gap", p_rel_gap));
}


void ILPSolverHighs::set_cutoff(double p_cutoff)
{
    ASSERT_OK(d_highs.setOptionValue("objective_bound", p_cutoff));
}


void ILPSolverHighs::print_mps_file(const std::string& p_filename)
{
    assert(p_filename.substr(p_filename.size() - 4, 4) == ".mps");
    ASSERT_OK(d_highs.writeModel(p_filename));
}


void ILPSolverHighs::add_variable_impl(VariableType p_type, double p_objective, double p_lower_bound,
                                       double p_upper_bound, const std::string& p_name, OptionalValueArray p_row_values,
                                       OptionalIndexArray p_row_indices)
{
    if (p_row_values)
    {
        if (p_row_indices) // Coefficients given as sparse vector.
        {
            assert(p_row_values->size() == p_row_indices->size());
            ASSERT_OK(d_highs.addCol(p_objective, p_lower_bound, p_upper_bound, isize(*p_row_values),
                                     p_row_indices->data(), p_row_values->data()));
        }
        else // Coefficients given as dense vector.
        {
            d_sparse.init_from_dense(*p_row_values);
            ASSERT_OK(d_highs.addCol(p_objective, p_lower_bound, p_upper_bound, isize(d_sparse),
                                     d_sparse.indices().data(), d_sparse.values().data()));
        }
    }
    else // No coefficients given.
    {
        assert(!p_row_indices);
        ASSERT_OK(d_highs.addCol(p_objective, p_lower_bound, p_upper_bound, 0, nullptr, nullptr));
    }

    const auto new_col_idx = d_highs.getNumCol() - 1;
    // HiGHS has no binary variables, so we use an integral variable bounded by 0 and 1.
    assert(p_type != VariableType::BINARY || (p_lower_bound == 0. && p_upper_bound == 1.));
    auto highs_type = p_type == VariableType::CONTINUOUS ? HighsVarType::kContinuous : HighsVarType::kInteger;
    ASSERT_OK(d_highs.changeColIntegrality(new_col_idx, highs_type));
    // HiGHS returns an error for empty names.
    if (!p_name.empty())
        ASSERT_OK(d_highs.passColName(new_col_idx, replace_spaces(p_name)));
}


void ILPSolverHighs::add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                         const std::string& p_name, OptionalIndexArray p_col_indices)
{
    if (p_col_indices) // Sparse value vector given
    {
        assert(p_col_values.size() == p_col_indices->size());
        ASSERT_OK(d_highs.addRow(p_lower_bound, p_upper_bound, isize(p_col_values), p_col_indices->data(),
                                 p_col_values.data()));
    }
    else // Dense value vector given.
    {
        d_sparse.init_from_dense(p_col_values);
        ASSERT_OK(d_highs.addRow(p_lower_bound, p_upper_bound, isize(d_sparse), d_sparse.indices().data(),
                                 d_sparse.values().data()));
    }
    // HiGHS returns an error for empty names.
    if (!p_name.empty())
        ASSERT_OK(d_highs.passRowName(d_highs.getNumRow() - 1, replace_spaces(p_name)));
}


void ILPSolverHighs::solve_impl()
{
    [[maybe_unused]] const auto status = d_highs.run();
    // status will be HighsStatus::kWarning if the function was aborted early
    // due to some time, iteration or solution limit. (See HiGHS internal function highsStatusFromHighsModelStatus.)
    // So we only assert that it is not an error.
    assert(status != HighsStatus::kError);
}


void ILPSolverHighs::set_objective_sense_impl(ObjectiveSense p_sense)
{
    ASSERT_OK(d_highs.changeObjectiveSense(p_sense == ObjectiveSense::MAXIMIZE ? ObjSense::kMaximize : ObjSense::kMinimize));
}

} // namespace ilp_solver

#endif
