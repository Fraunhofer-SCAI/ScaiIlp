#if defined(WITH_GUROBI) && (_WIN64 == 1)


#include "ilp_solver_gurobi.hpp"

#include "utility.hpp"

#include <algorithm>
#include <cassert>


namespace ilp_solver
{

#include "gurobi_c.h"
#define GRB_STRINGIZE2(s) #s
#define GRB_STRINGIZE(s)  GRB_STRINGIZE2(s)
#pragma comment(lib, "gurobi" GRB_STRINGIZE(GRB_VERSION_MAJOR) GRB_STRINGIZE(GRB_VERSION_MINOR) ".lib ")

namespace
{
    // A template instead of the macro expansion to handle return codes.
    // If the Gurobi function call was unsuccessful, throw an error.
    template<typename F, typename... Args>
    __forceinline void call_gurobi(GRBmodel* p_model, F p_f, Args... p_args)
    {
        auto retcode = p_f(p_args...);
        if (retcode != 0)
        {
            auto ret = std::string("Gurobi Error: \"") + GRBgeterrormsg(GRBgetenv(p_model)) + '"';
            throw std::runtime_error(ret);
        }
    }


    inline void update_index_vector(std::vector<int>& p_vec, int p_size)
    {
        const auto old_size{isize(p_vec)};
        if (p_size > old_size)
        {
            p_vec.reserve(p_size);
            for (int i = old_size; i < p_size; ++i)
                p_vec.push_back(i);
        }
    }
}


ILPSolverGurobi::ILPSolverGurobi()
{
    // We can not use call_gurobi before the model is set up.
    auto ret = GRBloadenv(&d_env, nullptr);
    if (ret != 0)
        throw std::runtime_error("Gurobi Error: \"Could not set up the environment.\"");

    ret = GRBnewmodel(d_env, &d_model, "", 0, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (ret != 0)
        throw std::runtime_error("Gurobi Error: \"Could not create a new model.\"");

    set_default_parameters(this);
}


ILPSolverGurobi::~ILPSolverGurobi()
{
    // Do not use `call_gurobi` and ignore errors so that this destructor does not throw.
    GRBfreemodel(d_model);
    GRBfreeenv(d_env);
}


int ILPSolverGurobi::get_num_constraints() const
{
    return d_num_cons;
}


int ILPSolverGurobi::get_num_variables() const
{
    return d_num_vars;
}


std::vector<double> ILPSolverGurobi::get_solution() const
{
    int sol_count{0};
    call_gurobi(d_model, GRBgetintattr, d_model, GRB_INT_ATTR_SOLCOUNT, &sol_count);
    if (sol_count > 0)
    {
        std::vector<double> solution(d_num_vars);
        call_gurobi(d_model, GRBgetdblattrarray, d_model, GRB_DBL_ATTR_X, 0, d_num_vars, solution.data());
        return solution;
    }
    return std::vector<double>();
}


std::vector<double> ILPSolverGurobi::get_dual_sol() const
{
    int sol_count{0};
    call_gurobi(d_model, GRBgetintattr, d_model, GRB_INT_ATTR_SOLCOUNT, &sol_count);
    if (sol_count > 0)
    {
        std::vector<double> dual_sol(d_num_cons);
        call_gurobi(d_model, GRBgetdblattrarray, d_model, GRB_DBL_ATTR_PI, 0, d_num_cons, dual_sol.data());
        return dual_sol;
    }
    return std::vector<double>();
}


double ILPSolverGurobi::get_objective() const
{
    double result{0.};
    call_gurobi(d_model, GRBgetdblattr, d_model, GRB_DBL_ATTR_OBJVAL, &result);
    return result;
}


SolutionStatus ILPSolverGurobi::get_status() const
{
    int status{0};
    call_gurobi(d_model, GRBgetintattr, d_model, GRB_INT_ATTR_STATUS, &status);
    int sol_count{0};
    call_gurobi(d_model, GRBgetintattr, d_model, GRB_INT_ATTR_SOLCOUNT, &sol_count);

    SolutionStatus current_status = (sol_count > 0) ? SolutionStatus::SUBOPTIMAL : SolutionStatus::NO_SOLUTION;
    switch (status)
    {
    case GRB_OPTIMAL:         return SolutionStatus::PROVEN_OPTIMAL;    // The only three cases were current_status
    case GRB_INFEASIBLE:      return SolutionStatus::PROVEN_INFEASIBLE; // does not hold the correct value.
    case GRB_UNBOUNDED:       return SolutionStatus::PROVEN_UNBOUNDED;
    case GRB_CUTOFF:          return SolutionStatus::PROVEN_INFEASIBLE;
    case GRB_LOADED:          [[fallthrough]];
    case GRB_INF_OR_UNBD:     [[fallthrough]];
    case GRB_ITERATION_LIMIT: [[fallthrough]];
    case GRB_NODE_LIMIT:      [[fallthrough]];
    case GRB_TIME_LIMIT:      [[fallthrough]];
    case GRB_SOLUTION_LIMIT:  [[fallthrough]];
    case GRB_INTERRUPTED:     [[fallthrough]];
    case GRB_NUMERIC:         [[fallthrough]];
    case GRB_SUBOPTIMAL:      [[fallthrough]];
    case GRB_INPROGRESS:      [[fallthrough]];
    case GRB_USER_OBJ_LIMIT:  [[fallthrough]];
    default:                  return current_status;
    }
}


void ILPSolverGurobi::set_start_solution(ValueArray p_solution)
{
    assert(isize(p_solution) == d_num_vars);
    // Check feasibility of the start solution by optimizing with all variables fixed and
    // throw an InvalidStartSolutionException if it is not.
    // Needs to backup and afterwards restore the lower and upper bounds on the variables.
    call_gurobi(d_model, GRBupdatemodel, d_model);
    std::vector<double> lb(d_num_vars);
    std::vector<double> ub(d_num_vars);
    call_gurobi(d_model, GRBgetdblattrarray, d_model, GRB_DBL_ATTR_LB,    0, d_num_vars, lb.data());
    call_gurobi(d_model, GRBgetdblattrarray, d_model, GRB_DBL_ATTR_UB,    0, d_num_vars, ub.data());
    call_gurobi(d_model, GRBsetdblattrarray, d_model, GRB_DBL_ATTR_LB,    0, d_num_vars, const_cast<double*>(p_solution.data()));
    call_gurobi(d_model, GRBsetdblattrarray, d_model, GRB_DBL_ATTR_UB,    0, d_num_vars, const_cast<double*>(p_solution.data()));
    solve_impl();
    SolutionStatus status = get_status();
    call_gurobi(d_model, GRBsetdblattrarray, d_model, GRB_DBL_ATTR_LB,    0, d_num_vars, lb.data());
    call_gurobi(d_model, GRBsetdblattrarray, d_model, GRB_DBL_ATTR_UB,    0, d_num_vars, ub.data());
    call_gurobi(d_model, GRBsetdblattrarray, d_model, GRB_DBL_ATTR_START, 0, d_num_vars, const_cast<double*>(p_solution.data()));
    if (status == SolutionStatus::PROVEN_INFEASIBLE)
        throw InvalidStartSolutionException();
}


void ILPSolverGurobi::reset_solution()
{
    call_gurobi(d_model, GRBreset, d_model, false);
}


void ILPSolverGurobi::set_num_threads(int p_num_threads)
{
    assert(p_num_threads >= 0);
    call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_THREADS, p_num_threads);
}


void ILPSolverGurobi::set_deterministic_mode(bool)
{
    // Gurobi is always deterministic (unless using concurrent MIP solving mode, which we do not do).
    // Therefore, this function is empty.
}


void ILPSolverGurobi::set_log_level(int p_level)
{
    p_level = (p_level < 0) ? 0 : p_level;
    if (p_level == 0)
        call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_OUTPUTFLAG, 0); // 0 means no output.
    else
    {
        call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_OUTPUTFLAG, 1);
        call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_LOGTOCONSOLE, 1);

        p_level = 1 + 9 / p_level;
        // Gurobi prints log lines every DisplayInterval seconds.
        // We chose 10 seconds as the maximum, while 1 second is the Gurobi minimum (reached for p_level > 9).
        call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_DISPLAYINTERVAL, p_level);
    }
}


void ILPSolverGurobi::set_presolve(bool p_preprocessing)
{
    // -1 is the automatic setting and Gurobi default, 0 disables presolving.
    call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_PRESOLVE, p_preprocessing ? -1 : 0);
}


void ILPSolverGurobi::set_max_seconds_impl(double p_seconds)
{
    assert(p_seconds >= 0.);
    call_gurobi(d_model, GRBsetdblparam, GRBgetenv(d_model), GRB_DBL_PAR_TIMELIMIT, p_seconds);
}


void ILPSolverGurobi::set_max_nodes(int p_nodes)
{
    assert(p_nodes >= 0);
    call_gurobi(d_model, GRBsetdblparam, GRBgetenv(d_model), GRB_DBL_PAR_NODELIMIT, p_nodes);
}


void ILPSolverGurobi::set_max_solutions(int p_solutions)
{
    p_solutions = std::clamp(p_solutions, 1, 2000000000); // Gurobi MAXINT
    call_gurobi(d_model, GRBsetintparam, GRBgetenv(d_model), GRB_INT_PAR_SOLUTIONLIMIT, p_solutions);
}


void ILPSolverGurobi::set_max_abs_gap(double p_abs_gap)
{
    assert(p_abs_gap >= 0.);
    call_gurobi(d_model, GRBsetdblparam, GRBgetenv(d_model), GRB_DBL_PAR_MIPGAPABS, p_abs_gap);
}


void ILPSolverGurobi::set_max_rel_gap(double p_rel_gap)
{
    assert(p_rel_gap >= 0.);
    call_gurobi(d_model, GRBsetdblparam, GRBgetenv(d_model), GRB_DBL_PAR_MIPGAP, p_rel_gap);
}


void ILPSolverGurobi::set_cutoff(double p_cutoff)
{
    // Only set cutoff if intended. Otherwise stick to the Gurobi default.
    if (p_cutoff != c_default_cutoff)
        call_gurobi(d_model, GRBsetdblparam, GRBgetenv(d_model), GRB_DBL_PAR_CUTOFF, p_cutoff);
}


void ILPSolverGurobi::print_mps_file(const std::string& p_filename)
{
    assert(p_filename.substr(p_filename.size() - 4, 4) == ".mps");
    call_gurobi(d_model, GRBwrite, d_model, p_filename.c_str());
}


void ILPSolverGurobi::add_variable_impl(VariableType p_type, double p_objective, double p_lower_bound,
                                        double p_upper_bound, const std::string& p_name,
                                        OptionalValueArray p_row_values, OptionalIndexArray p_row_indices)
{
    int     num{0};
    int*    indices{nullptr};
    double* values{nullptr};
    if (p_row_values)
    {
        if (p_row_indices)
        {
            num     = isize(*p_row_indices);
            indices = const_cast<int*>(p_row_indices->data());
        }
        else
        {
            num = d_num_cons;
            update_index_vector(d_indices, num);
            indices = const_cast<int*>(d_indices.data());
        }

        values = const_cast<double*>(p_row_values->data());
        assert(isize(*p_row_values) == num);
    }

    char type = (p_type == VariableType::INTEGER)    ? GRB_INTEGER
              : (p_type == VariableType::CONTINUOUS) ? GRB_CONTINUOUS
                                                     : GRB_BINARY;

    call_gurobi(d_model, GRBaddvar, d_model, num, indices, values, p_objective, p_lower_bound, p_upper_bound, type, p_name.c_str());
    ++d_num_vars;
}


void ILPSolverGurobi::add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                          const std::string& p_name, OptionalIndexArray p_col_indices)
{
    int     num{0};
    int*    indices{nullptr};
    double* values{const_cast<double*>(p_col_values.data())};

    if (p_col_indices)
    {
        num     = isize(*p_col_indices);
        indices = const_cast<int*>(p_col_indices->data());

        assert(p_col_indices->size() == p_col_values.size());
        assert(num <= d_num_vars);
    }
    else
    {
        num = d_num_vars;
        update_index_vector(d_indices, num);
        indices = const_cast<int*>(d_indices.data());

        assert(isize(p_col_values) == num);
    }

    if (p_lower_bound == p_upper_bound)
    {
        call_gurobi(d_model, GRBaddconstr, d_model, num, indices, values, GRB_EQUAL, p_lower_bound, p_name.c_str());
        ++d_num_cons;
    }
    else if (p_lower_bound >= c_neg_inf_bound && p_upper_bound <= c_pos_inf_bound)
    {
        call_gurobi(d_model, GRBaddrangeconstr, d_model, num, indices, values, p_lower_bound, p_upper_bound, p_name.c_str());
        ++d_num_cons;
    }
    else if (p_lower_bound >= c_neg_inf_bound && p_upper_bound > c_pos_inf_bound)
    {
        call_gurobi(d_model, GRBaddconstr, d_model, num, indices, values, GRB_GREATER_EQUAL, p_lower_bound, p_name.c_str());
        ++d_num_cons;
    }
    else if (p_lower_bound < c_neg_inf_bound && p_upper_bound <= c_pos_inf_bound)
    {
        call_gurobi(d_model, GRBaddconstr, d_model, num, indices, values, GRB_LESS_EQUAL, p_upper_bound, p_name.c_str());
        ++d_num_cons;
    }
}


void ILPSolverGurobi::solve_impl()
{
    call_gurobi(d_model, GRBoptimize, d_model);
}


void ILPSolverGurobi::set_objective_sense_impl(ObjectiveSense p_sense)
{
    int sense{(p_sense == ObjectiveSense::MINIMIZE) ? 1 : -1};
    call_gurobi(d_model, GRBsetintattr, d_model, GRB_INT_ATTR_MODELSENSE, sense);
}
}

#endif
