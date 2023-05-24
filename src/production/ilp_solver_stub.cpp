#include "ilp_solver_stub.hpp"

#include "ilp_solver_interface.hpp"
#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>

namespace ilp_solver
{
    static std::chrono::milliseconds seconds_to_millisecods(double p_seconds)
    {
        const auto     milliseconds = 1000. * p_seconds;
        constexpr auto max_milliseconds = std::numeric_limits<std::chrono::milliseconds::rep>::max();
        if (milliseconds >= max_milliseconds)
            return std::chrono::milliseconds(max_milliseconds);
        return std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(milliseconds));
    }


    static std::string exit_code_to_message(SolverExitCode p_exit_code)
    {
        switch (p_exit_code)
        {
        case SolverExitCode::ok:
            return "";
        case SolverExitCode::killed_via_task_manager:
            return "ScaiIlp killed.";
        case SolverExitCode::uncaught_exception_1:
            return "Uncaught exception, likely out of memory (stack buffer overflow Windows 7).";
        case SolverExitCode::uncaught_exception_2:
            return "Uncaught exception, likely out of memory (C++ exception).";
        case SolverExitCode::uncaught_exception_3:
            return "Uncaught exception, likely out of memory (stack buffer overflow Windows 10).";
        case SolverExitCode::uncaught_exception_4:
            return "Uncaught exception, the heap was most likely filled or corrupted.";
        case SolverExitCode::uncaught_exception_5:
            return "Uncaught exception: Access violation.";
        case SolverExitCode::missing_dll:
            return "DLL missing";
        case SolverExitCode::out_of_memory:
            return "Out of memory.";
        case SolverExitCode::command_line_error:
            return "Invalid command line.";
        case SolverExitCode::shared_memory_error:
            return "Failed communicating via shared memory.";
        case SolverExitCode::model_error:
            return "Failed generating model.";
        case SolverExitCode::solver_error:
            return "Failed solving (solver error).";
        case SolverExitCode::forced_termination:
            return "Failed solving (timeout).";
        default:
            return "Unknown exit code " + std::to_string(static_cast<int>(p_exit_code)) + ".";
        }
    }


    static bool exit_code_should_be_ignored_silently(SolverExitCode p_exit_code)
    {
        switch (p_exit_code)
        {
        case SolverExitCode::out_of_memory:
        case SolverExitCode::uncaught_exception_1:
        case SolverExitCode::uncaught_exception_2:
        case SolverExitCode::uncaught_exception_3:
        case SolverExitCode::uncaught_exception_4:
        case SolverExitCode::uncaught_exception_5:
        case SolverExitCode::forced_termination:
            return true;
        default:
            return false;
        }
    }


    // set_default_parameters is called in ILPSolverCollect.
    ILPSolverStub::ILPSolverStub(const std::string& p_executable_basename, bool p_throw_on_all_crashes)
        : d_executable_basename(p_executable_basename), d_throw_on_all_crashes(p_throw_on_all_crashes)
    { }


    std::vector<double> ILPSolverStub::get_solution() const
    {
        return d_ilp_solution_data.solution;
    }


    double ILPSolverStub::get_objective() const
    {
        return d_ilp_solution_data.objective;
    }


    SolutionStatus ILPSolverStub::get_status() const
    {
        return d_ilp_solution_data.solution_status;
    }


    void ILPSolverStub::reset_solution()
    {
        d_ilp_data.start_solution.clear();
        d_ilp_solution_data = ILPSolutionData(d_ilp_data.objective_sense);
    }


    void ILPSolverStub::solve_impl()
    {
        d_ilp_solution_data = ILPSolutionData(d_ilp_data.objective_sense);

        CommunicationParent communicator;
        const auto          shared_memory_name = communicator.write_ilp_data(d_ilp_data);
        // We expect the ScaiILP executable lying next to the one calling it.
        const auto full_executable_path = boost::dll::program_location().parent_path() / d_executable_basename;
        // Start the process. We use path::native to ensure supporting unicode paths on windows.
        auto proc = boost::process::child(full_executable_path.native(), shared_memory_name);
        // We wait for the process to complete for 1.5 times longer than we allow the solver to compute.
        // If the process did not finish by then, we forcibly terminate it.
        if (!proc.wait_for(seconds_to_millisecods(1.5 * std::max(1.0, d_ilp_data.max_seconds))))
            proc.terminate();
        auto exit_code = SolverExitCode(proc.exit_code());

        if (d_ilp_data.log_level)
            std::cout << "External Solver messages: \"" << exit_code_to_message(exit_code) << "\" (Exit Code "
                      << static_cast<int>(exit_code) << ")\n";

        communicator.read_solution_data(&d_ilp_solution_data);
        if (exit_code == SolverExitCode::missing_dll) // missing DLL should be a Runtime Error
            throw ilp_solver::SolverExeException(("External ILP solver: " + exit_code_to_message(exit_code)).c_str());
        if (exit_code != SolverExitCode::ok && (d_throw_on_all_crashes || !exit_code_should_be_ignored_silently(exit_code)))
            throw std::exception(("External ILP solver: " + exit_code_to_message(exit_code)).c_str());
    }
}
