#include "ilp_solver_stub.hpp"

#include "ilp_solver_interface.hpp"
#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"

#include <cassert>
#include <chrono>
#include <format>
#include <iostream>
#include <stdexcept>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>

namespace ilp_solver
{
// In AXS-1452, we introduced a wait time limit because we observed CBC not terminating after hours
// despite a given time limit of minutes.
// We now wait for (max_seconds plus some overtime) =: wait_max_seconds.
// When wait_max_seconds is exceeded, the external process is killed, but the intermediate result reached is preserved.
// This is more convenient than letting the user kill the external process or even the calling process.
// (The latter would lose the intermediate result).
//
// In AXS-1452, we started with relative_overtime=0.5.
// We hoped that this would be large enough to always terminate regularly.
// Also we had recommended one of our users to kill the process after 2*max_seconds,
// so we wanted a smaller wait_max_seconds.
//
// After AXS-2636 we observed that occasionally CBC still ran into wait_max_seconds. So we added absolute_overtime.
// Unlike the initial instance from AXS-1452, the runtime of the instance of AXS-2636 seems very volatile.
// With time_limit=20s (60s distributed to 3 calls), overtime ranges from <2s total to >10s for on one run.
//
// Current values are experimental.
constexpr auto c_relative_overtime         = 0.5;
constexpr auto c_absolute_overtime_seconds = 10.0;


static std::chrono::milliseconds seconds_to_millisecods(double p_seconds)
{
    const auto     milliseconds     = 1000. * p_seconds;
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
        return "Unexpected exit code \"forced termination\"."; // If forced termination by stub occurs, we do not
                                                                // call exit_code_to_message. So the exit code is
                                                                // unexpected here.
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
    SolverExitCode exit_code{};
    std::string    exit_message{};

    try
    {
        d_ilp_solution_data = ILPSolutionData(d_ilp_data.objective_sense);

        CommunicationParent communicator;
        const auto          shared_memory_name = communicator.write_ilp_data(d_ilp_data);
        // We expect the ScaiILP executable lying next to the one calling it.
        const auto full_executable_path = boost::dll::program_location().parent_path() / d_executable_basename;
        // Start the process. If the log level is zero, suppress all of its output.
        // Ideally, suppressing the output should not be necessary,
        // but we have repeatedly observed CBC writing to stdout at log level zero.
        auto proc = d_ilp_data.log_level != 0 ? boost::process::child(full_executable_path, shared_memory_name)
                                              : boost::process::child(full_executable_path, shared_memory_name,
                                                                      boost::process::std_out > boost::process::null,
                                                                      boost::process::std_err > boost::process::null);
        // Wait hopefully long enough. Kill child if time limit is exceeded. See comment on c_timeout_factor.
        const auto wait_max_seconds = (1.0 + c_relative_overtime) * d_ilp_data.max_seconds + c_absolute_overtime_seconds;
        if (!proc.wait_for(seconds_to_millisecods(wait_max_seconds)))
        {
            proc.terminate(); // boost::process seems not to support to set the exit code by terminate().
                              // Note that terminate(error_code&) does not set the exit code either, but has a different purpose.
            exit_code = SolverExitCode::forced_termination; // Don't read the exit code, but set it manually to the fixed desired value.
            exit_message = std::format("Failed solving by timeout. (limit:{} timeout:{})", d_ilp_data.max_seconds,
                                       wait_max_seconds);
        }
        else
        {
            exit_code    = SolverExitCode(proc.exit_code());
            exit_message = exit_code_to_message(exit_code);
        }

        if (d_ilp_data.log_level)
            std::cout << "External Solver messages: \"" << exit_message
                      << "\" (Exit Code " << static_cast<int>(exit_code) << ")\n";

        communicator.read_solution_data(&d_ilp_solution_data);
    }
    // Rethrow all exceptions as SolverExeExceptions, so they can be easily traced back to this function.
    catch (const std::exception& p_e)
    {
        throw SolverExeException(p_e.what());
    }
    catch (...)
    {
        throw SolverExeException("Unknown Error.");
    }

    if (exit_code != SolverExitCode::ok && (d_throw_on_all_crashes || !exit_code_should_be_ignored_silently(exit_code)))
        throw SolverExeException(exit_message);
}

} // namespace ilp_solver
