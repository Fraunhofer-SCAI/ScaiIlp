#include "ilp_data.hpp"
#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"
#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"

#include <stdexcept>
#include <string>

#include <windows.h>    // for SetErrorMode

using namespace ilp_solver;

class ModelException : public std::exception {};
class SolverException : public std::exception {};

using ilp_solver::ILPSolverInterface;


static void add_variables(ILPSolverInterface* v_solver, const ILPData& p_data)
{
    const auto num_variables = (int) p_data.variable_type.size();

    for (auto variable_idx = 0; variable_idx < num_variables; ++variable_idx)
    {
        const auto lower = p_data.variable_lower[variable_idx];
        const auto upper = p_data.variable_upper[variable_idx];
        const auto variable_type = p_data.variable_type[variable_idx];
        const auto objective = p_data.objective[variable_idx];

        if (variable_type == VariableType::INTEGER)
            v_solver->add_variable_integer(objective, lower, upper);
        else
            v_solver->add_variable_continuous(objective, lower, upper);
    }
}


static void add_constraints(ILPSolverInterface* v_solver, const ILPData& p_data)
{
    const auto num_constraints = (int) p_data.matrix.size();

    for (auto i = 0; i < num_constraints; ++i)
    {
        const auto& row = p_data.matrix[i];
        const auto lower = p_data.constraint_lower[i];
        const auto upper = p_data.constraint_upper[i];

        v_solver->add_constraint(row, lower, upper);
    }
}


static void generate_ilp(ILPSolverInterface* v_solver, const ILPData& p_data)
{
    add_variables(v_solver, p_data);
    add_constraints(v_solver, p_data);
}


static void set_solver_preparation_parameters(ILPSolverInterface* v_solver, const ILPData& p_data)
{
    if (!p_data.start_solution.empty())
        v_solver->set_start_solution(p_data.start_solution);
}


static void set_solver_parameters(ILPSolverInterface* v_solver, const ILPData& p_data)
{
    v_solver->set_num_threads       (p_data.num_threads);
    v_solver->set_deterministic_mode(p_data.deterministic);
    v_solver->set_log_level         (p_data.log_level);
    v_solver->set_presolve          (p_data.presolve);

    v_solver->set_max_seconds       (p_data.max_seconds);
    v_solver->set_max_nodes         (p_data.max_nodes);
    v_solver->set_max_solutions     (p_data.max_solutions);
    v_solver->set_max_abs_gap       (p_data.max_abs_gap);
    v_solver->set_max_rel_gap       (p_data.max_rel_gap);
}


static void solve_ilp(ILPSolverInterface* v_solver, ObjectiveSense p_objective_sense)
{
    if (p_objective_sense == ObjectiveSense::MINIMIZE)
        v_solver->minimize();
    else
        v_solver->maximize();
}


static ILPSolutionData solution_data(const ILPSolverInterface& p_solver)
{
    ILPSolutionData solution_data;

    solution_data.solution        = p_solver.get_solution();
    solution_data.objective       = p_solver.get_objective();
    solution_data.solution_status = p_solver.get_status();

    return solution_data;
}


// Throws ModelException, SolverException or std::bad_alloc
static ILPSolutionData solve_ilp(const ILPData& p_data)
{
    auto solver = ilp_solver::create_solver_cbc();


    // RAII for deleting solver
    struct SolverDeleter
    {
        explicit SolverDeleter(ILPSolverInterface* p_solver) : solver(p_solver) {}
        ~SolverDeleter()                                               { ilp_solver::destroy_solver(solver); }

        ILPSolverInterface* solver;
    } solver_deleter(solver);

    try
    {
        generate_ilp(solver, p_data);
        set_solver_preparation_parameters(solver, p_data);
        set_solver_parameters(solver, p_data);
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)                   { throw ModelException(); }

    try
    {
        solve_ilp(solver, p_data.objective_sense);

        return solution_data(*solver);
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)                   { throw SolverException(); }
}


static SolverExitCode solve_ilp(const std::string& p_shared_memory_name)
{
    try
    {
        ILPData data;

        CommunicationChild communicator(p_shared_memory_name);
        communicator.read_ilp_data(&data);

        auto solution_data = solve_ilp(data);

        communicator.write_solution_data(solution_data);

        return SolverExitCode::ok;
    }
    catch (const std::bad_alloc&)   { return SolverExitCode::out_of_memory;       }
    catch (const ModelException&)   { return SolverExitCode::model_error;         }
    catch (const SolverException&)  { return SolverExitCode::solver_error;        }
    catch (...)                     { return SolverExitCode::shared_memory_error; }
}


SolverExitCode my_main (int argc, wchar_t* argv[])
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc != 2)
        return SolverExitCode::command_line_error;
    const auto shared_memory_name = std::wstring(argv[1]);
    return solve_ilp(utf16_to_utf8(shared_memory_name));
}


int wmain(int argc, wchar_t* argv[])
{
    return static_cast<int>(my_main (argc, argv));
}
