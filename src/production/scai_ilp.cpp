#include "ilp_data.hpp"
#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"
#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"
#include "utility.hpp"

#include <boost/chrono.hpp>
#include <boost/nowide/convert.hpp>
#include <chrono>
#include <stdexcept>
#include <string>

#include <windows.h> // for SetErrorMode
#include <psapi.h> // GetProcessMemoryInfo

#ifdef WITH_MIMALLOC
#pragma warning(push)
#pragma warning(disable : 28251) // inconsistent annotation
#pragma warning(disable : 4559)  // redefinition with __declspec(restrict)

// Include the new-delete header to make their redirections use mimalloc directly for efficiency,
// c.f.https://github.com/microsoft/mimalloc
#include "mimalloc-new-delete.h"
// Include the mimalloc header and call mi_version to ensure that the dll is actually imported
// and not skipped due to not being used.
#include "mimalloc.h"

const inline int c_mimalloc_version = mi_version();
#pragma warning(pop)
#endif

using namespace ilp_solver;

class ModelException  : public std::exception {};
class SolverException : public std::exception {};

using ilp_solver::ScopedILPSolver;

using Seconds   = boost::chrono::duration<double>;
using UserClock = boost::chrono::process_user_cpu_clock;

// 0 never crash on purpose
// 1 crash on large LPs
// 2 always crash
constexpr int  c_test_crash = 0;
constexpr auto c_test_exit_code
//= SolverExitCode::out_of_memory; // results in warning if only large LPs fail
//= SolverExitCode::missing_dll; // always results in error
= SolverExitCode::forced_termination; // special case

static void add_variables(ScopedILPSolver& v_solver, const ILPDataView& p_data)
{
    const auto num_variables = isize(p_data.variable_type);

    for (auto variable_idx = 0; variable_idx < num_variables; ++variable_idx)
    {
        const auto lower = p_data.variable_lower[variable_idx];
        const auto upper = p_data.variable_upper[variable_idx];
        const auto variable_type = p_data.variable_type[variable_idx];
        const auto objective = p_data.objective[variable_idx];

        if (variable_type == VariableType::INTEGER)
            v_solver->add_variable_integer(objective, lower, upper);
        else if (variable_type == VariableType::BINARY)
            v_solver->add_variable_boolean(objective);
        else
            v_solver->add_variable_continuous(objective, lower, upper);
    }
}


static void add_constraints(ScopedILPSolver& v_solver, const ILPDataView& p_data)
{
    const auto num_constraints = isize(p_data.matrix.d_values);

    for (auto i = 0; i < num_constraints; ++i)
    {
        const auto& values  = p_data.matrix.d_values [i];
        const auto& indices = p_data.matrix.d_indices[i];
        const auto  lower   = p_data.constraint_lower[i];
        const auto  upper   = p_data.constraint_upper[i];

        v_solver->add_constraint(indices, values, lower, upper);
    }
}


static void generate_ilp(ScopedILPSolver& v_solver, const ILPDataView& p_data)
{
    add_variables  (v_solver, p_data);
    add_constraints(v_solver, p_data);
}


static void set_solver_preparation_parameters(ScopedILPSolver& v_solver, const ILPDataView& p_data)
{
    if (!p_data.start_solution.empty())
        v_solver->set_start_solution(p_data.start_solution);
}


static void set_solver_parameters(ScopedILPSolver& v_solver, const ILPDataView& p_data)
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
    v_solver->set_cutoff            (p_data.cutoff);
}


static void solve_ilp(ScopedILPSolver& v_solver, ObjectiveSense p_objective_sense)
{
    if (p_objective_sense == ObjectiveSense::MINIMIZE)
        v_solver->minimize();
    else
        v_solver->maximize();
}


// Returns peak memory usage in megabytes.
static double peak_memory_usage()
{
    PROCESS_MEMORY_COUNTERS memory;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
        return static_cast<unsigned long long>(memory.PeakWorkingSetSize) * 0x1p-20;
    return 0.;
}


static ILPSolutionData solution_data(const ScopedILPSolver& p_solver, UserClock::time_point p_start_time)
{
    ILPSolutionData solution_data;

    solution_data.solution        = p_solver->get_solution();
    solution_data.dual_sol        = p_solver->get_dual_sol();
    solution_data.objective       = p_solver->get_objective();
    solution_data.solution_status = p_solver->get_status();
    solution_data.peak_memory     = peak_memory_usage();
    solution_data.cpu_time_sec    = Seconds(UserClock::now() - p_start_time).count();

    return solution_data;
}


// Throws ModelException, InvalidStartSolutionException, SolverException or std::bad_alloc
static ILPSolutionData solve_ilp(const ILPDataView& p_data, CommunicationChild& p_communicator)
{
    const auto start_time = UserClock::now();
    auto       solver     = std::get<0>(all_solvers[1])();

    try
    {
        generate_ilp(solver, p_data);
        set_solver_preparation_parameters(solver, p_data);
        set_solver_parameters(solver, p_data);
        solver->set_interim_results([&p_communicator](ILPSolutionData* p_solution) -> void
        {
            p_communicator.write_solution_data(*p_solution);
        }); // Save interim results in case the solver crashes.
        // If the solver never finds a solution better than the start solution, above callback is never called.
        // So, we manually ensure that at least the start solution is communicated back to the calling process.
        p_communicator.write_solution_data(solution_data(solver, start_time));
    }
    catch (const std::bad_alloc&)                { throw; }
    catch (const InvalidStartSolutionException&) { throw; }
    catch (...)                                  { throw ModelException(); }

    try
    {
        solve_ilp(solver, p_data.objective_sense);
        return solution_data(solver, start_time);
    }
    catch (const std::bad_alloc&) { throw; }
    catch (...)                   { throw SolverException(); }
}


static SolverExitCode solve_ilp(const std::string& p_shared_memory_name)
{
    try
    {
        // read input data
        CommunicationChild communicator(p_shared_memory_name);
        auto               data = communicator.read_ilp_data();

        // test behavior of Caller when ScaiIlpExe crashes
        constexpr auto c_size_of_stub_tester = 2;
        if constexpr (c_test_crash != 0 && c_test_exit_code != SolverExitCode::forced_termination)
        {
            if constexpr (c_test_crash == 2)
                return c_test_exit_code;
            else if (data.matrix.d_num_cols > c_size_of_stub_tester)
                return c_test_exit_code;
        }

        // do the computation
        communicator.write_solution_data(solve_ilp(data, communicator));

        // test timeouts
        if constexpr (c_test_crash != 0 && c_test_exit_code == SolverExitCode::forced_termination)
        {
            if constexpr (c_test_crash == 2)
                std::this_thread::sleep_for(std::chrono::hours(8));
            else if (data.matrix.d_num_cols > c_size_of_stub_tester)
                std::this_thread::sleep_for(std::chrono::hours(8));
        }

        return SolverExitCode::ok;
    }
    catch (const std::bad_alloc&)                { return SolverExitCode::out_of_memory;          }
    catch (const InvalidStartSolutionException&) { return SolverExitCode::invalid_start_solution; }
    catch (const ModelException&)                { return SolverExitCode::model_error;            }
    catch (const SolverException&)               { return SolverExitCode::solver_error;           }
    catch (...)                                  { return SolverExitCode::shared_memory_error;    }
}


SolverExitCode my_main(int argc, wchar_t* argv[])
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc != 2)
        return SolverExitCode::command_line_error;
    const auto shared_memory_name = std::wstring(argv[1]);
    return solve_ilp(boost::nowide::narrow(shared_memory_name));
}


int wmain(int argc, wchar_t* argv[])
{
    return static_cast<int>(my_main (argc, argv));
}
