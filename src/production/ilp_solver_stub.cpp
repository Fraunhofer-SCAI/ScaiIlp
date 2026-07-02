#ifdef WITH_STUB

#include "ilp_solver_stub.hpp"

#include "ilp_solver_interface.hpp"
#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"
#include "tester.hpp"

#include <cassert>
#include <chrono>
#include <format>
#include <iostream>
#include <stdexcept>

#define NOMINMAX // asio leaks min/max macros from windows headers without this
#include <boost/asio.hpp>
#include <boost/process.hpp>
// Avoid unresolved symbols with boost process in boost 1.90, cmp. https://github.com/boostorg/process/issues/385
#pragma comment(lib, "ntdll.lib")

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


// Convert double seconds to std::chrono::steady_clock::duration, possibly capping at the maximal representable value.
// Note: We use the steady clock duration (usually nanoseconds) here, because asio uses it internally
//       and passing it the maximal amount of milliseconds would cause an overflow.
static auto sec_to_steady_clock_duration(double p_seconds)
{
    using Duration           = std::chrono::steady_clock::duration;
    const auto     ticks     = Duration::period::den * p_seconds;
    constexpr auto max_ticks = std::numeric_limits<Duration::rep>::max();
    if (ticks >= static_cast<double>(max_ticks))
        return Duration(max_ticks);
    return Duration(static_cast<Duration::rep>(ticks));
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
        return "Uncaught exception, likely a failed assert or out of memory (stack buffer overflow Windows 10).";
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
    case SolverExitCode::invalid_start_solution:
        return "Invalid start solution.";
    case SolverExitCode::stub_tester_failed:
        return "stub_tester failed.";
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


void ILPSolverStub::reset_solution()
{
    d_ilp_data.start_solution.clear();
    d_ilp_solution_data = ILPSolutionData(d_ilp_data.objective_sense);
}


void ILPSolverStub::solve_impl()
{
    std::string exit_message{};

    try
    {
        d_ilp_solution_data = ILPSolutionData(d_ilp_data.objective_sense);

        CommunicationParent communicator;
        const auto          shared_memory_name = communicator.write_ilp_data(d_ilp_data);

        namespace asio = boost::asio;
        namespace bp   = boost::process;
        // We expect the ScaiILP executable lying next to the one calling it.
        const auto full_executable_path = bp::ext::exe(bp::current_pid()).parent_path() / d_executable_basename;
        // Start the process. If the log level is zero, suppress all of its output.
        // Ideally, suppressing the output should not be necessary,
        // but we have repeatedly observed CBC writing to stdout at log level zero.
        auto stdio = d_ilp_data.log_level != 0
                       ? bp::process_stdio{}
                       : bp::process_stdio{{} /*stdin*/, nullptr /*stdout*/, nullptr /*stderr*/};
        auto ctx  = asio::io_context{};
        auto proc = bp::process(ctx, full_executable_path, {shared_memory_name}, stdio);
        const auto wait_max_seconds = (1.0 + c_relative_overtime) * d_ilp_data.max_seconds + c_absolute_overtime_seconds;
        auto set_exit_code = [&](boost::system::error_code p_ec, int p_exitcode)
        {
            if (p_ec == asio::error::operation_aborted)
            {
                d_exit_code  = SolverExitCode::forced_termination;
                exit_message = std::format("Failed solving by timeout. (limit:{} timeout:{})",
                                            d_ilp_data.max_seconds, wait_max_seconds);
            }
            else
            {
                d_exit_code  = SolverExitCode(p_exitcode);
                exit_message = exit_code_to_message(d_exit_code);
            }
        };
        // In case of a timeout the async_wait is cancelled. Note that this does not terminate the process!
        // The process is terminated at the end of the scope in this case.
        // It would be better to use:
        // bp::async_execute(std::move(proc))(asio::cancel_after(sec_to_steady_clock_duration(wait_max_seconds)))(set_exit_code);
        // But as of Boost 1.90, this does not set the error code if the timeout was reached.
        // See https://github.com/boostorg/process/issues/534
        proc.async_wait(asio::cancel_after(sec_to_steady_clock_duration(wait_max_seconds)))(set_exit_code);
        ctx.run();

        if (d_ilp_data.log_level)
            std::cout << std::format("External Solver messages: \"{}\" (Exit Code {})\n", exit_message, static_cast<int>(d_exit_code));

        // If the child process timed out, reading the solution data is not safe.
        // The child process may be writing to the shared memory right now.
        // Note that terminating the child process would not solve that either, as we may terminate it while it is writing.
        // If we do not read the solution data here, we lose intermediate results.
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
    // This is a logic error and not a runtime_error and should be rethrown here as such.
    if (d_exit_code == SolverExitCode::invalid_start_solution)
        throw InvalidStartSolutionException();

    // if exit_code is a candidate to be ignored silently
    if (!d_throw_on_all_crashes && exit_code_should_be_ignored_silently(d_exit_code))
    {
        // if the stub does not work even on a very simple LP, the installation is broken
        if (auto stub_tester_exit_code = stub_tester(d_executable_basename); stub_tester_exit_code != SolverExitCode::ok)
        {
            exit_message = std::format("{}\nMessage from stub_tester: {}\nPreceding original message: {}",
                                       exit_code_to_message(SolverExitCode::stub_tester_failed),
                                       exit_code_to_message(stub_tester_exit_code),
                                       exit_code_to_message(d_exit_code));
            d_exit_code  = SolverExitCode::stub_tester_failed;
            throw SolverExeException(exit_message);
        }
        // otherwise, we keep d_exit_code, but do not throw
    }
    else if (d_exit_code != SolverExitCode::ok)
        throw SolverExeException(exit_message);
}

} // namespace ilp_solver

#endif
