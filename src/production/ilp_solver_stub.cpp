#include "ilp_solver_stub.hpp"

#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>    // for GetModuleFileNameW, CreateProcessW etc.

constexpr auto c_file_separator  = "\\";
constexpr auto c_max_path_length = 1 << 16;

using std::string;
using std::wstring;

namespace ilp_solver
{
    static string quote(const string& p_string)
    {
        return '"' + p_string + '"';
    }


    static string directory_name(string p_filename)
    {
        const auto file_separator_pos = p_filename.rfind(c_file_separator);
        assert(file_separator_pos != std::wstring::npos);
        p_filename.erase(file_separator_pos);
        return p_filename + c_file_separator;
    }


    static string executable_dir()
    {
        char exe_path[c_max_path_length];
        GetModuleFileNameA(NULL, exe_path, c_max_path_length);
        return directory_name(exe_path);
    }


    static string full_executable_name(const string& p_basename)
    {
        return executable_dir() + p_basename;
    }


    static SolverExitCode execute_process(const string& p_executable_basename, const string& p_parameter, int p_wait_milliseconds)
    {
        // prepare command line
        const auto executable = full_executable_name(p_executable_basename);
        if (!std::filesystem::exists(executable))
            throw SolverExeException(("Could not find " + p_executable_basename).c_str());
        auto command_line = quote(executable) + ' ' + quote(p_parameter);

        // prepare parameters
        STARTUPINFOA startup_info;
        PROCESS_INFORMATION process_info;
        std::memset(&startup_info, 0, sizeof(startup_info));
        std::memset(&process_info, 0, sizeof(process_info));
        startup_info.cb = sizeof(startup_info);

        // start process
        if (!CreateProcessA(executable.c_str(),             // lpApplicationName
                            command_line.data(),            // lpCommandLine
                            0,                              // lpProcessAttributes
                            0,                              // lpThreadAttributes
                            false,                          // bInheritHandles
                            CREATE_DEFAULT_ERROR_MODE ,     // dwCreationFlags
                            0,                              // lpEnvironment
                            0,                              // lpCurrentDirectory
                            &startup_info,
                            &process_info))
            throw SolverExeException(("Error starting " + p_executable_basename + ". Error code:" + std::to_string(GetLastError())).c_str());

        // close handles via RAII
        struct HandleCloser
        {
            explicit HandleCloser(PROCESS_INFORMATION* p_process_info) : process_info(p_process_info) {}
            ~HandleCloser()
            {
                CloseHandle(process_info->hProcess);
                CloseHandle(process_info->hThread);
            }

            PROCESS_INFORMATION* process_info;
        } handle_closer(&process_info);

        // wait for the process to terminate
        auto return_code = WaitForSingleObject(process_info.hProcess, p_wait_milliseconds);
        switch (return_code) // according to https://msdn.microsoft.com/de-de/library/windows/desktop/ms687032%28v=vs.85%29.aspx, WaitForSingeObject can return the 4 values listed below
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            TerminateProcess(process_info.hProcess, static_cast<DWORD>(SolverExitCode::forced_termination));
            break;
        case WAIT_ABANDONED:
        case WAIT_FAILED:
        default: // we also handle unexpected values, just in case
            TerminateProcess(process_info.hProcess, static_cast<DWORD>(SolverExitCode::forced_termination));
            throw std::exception (("Error running " + p_executable_basename + ". Unexpected return code of WaitForSingleObject.").c_str());
        }

        // read process exit code
        DWORD exit_code;
        if (GetExitCodeProcess(process_info.hProcess, &exit_code))
            return SolverExitCode(exit_code);
        else
            throw std::exception(("Error obtaining exit code from " + p_executable_basename + ". Error code: " + std::to_string(GetLastError())).c_str());
    }


    int seconds_to_milliseconds (double p_seconds)
    {
        p_seconds *= 1000;
        if (p_seconds > std::numeric_limits<int>::max())
            return std::numeric_limits<int>::max();
        else
            return static_cast<int>(p_seconds);
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
    ILPSolverStub::ILPSolverStub(const std::string& p_executable_basename)
        : d_executable_basename(p_executable_basename)
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
        const auto shared_memory_name = communicator.write_ilp_data(d_ilp_data);

        auto exit_code = execute_process(d_executable_basename, shared_memory_name, seconds_to_milliseconds (1.5 * d_ilp_data.max_seconds));

        if (d_ilp_data.log_level)
            std::cout << "External Solver messages: \"" << exit_code_to_message(exit_code) << "\" (Exit Code " << static_cast<int>(exit_code) << ")\n";

        communicator.read_solution_data(&d_ilp_solution_data);

        if (exit_code != SolverExitCode::ok && !exit_code_should_be_ignored_silently(exit_code))
            throw std::exception(("External ILP solver: " + exit_code_to_message(exit_code)).c_str());
    }
}
