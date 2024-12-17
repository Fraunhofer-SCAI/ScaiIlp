#include "ilp_solver_stub.hpp"

#include "shared_memory_communication.hpp"
#include "solver_exit_code.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#define NOMINMAX
#include <windows.h>    // for GetModuleFileNameW, CreateProcessW etc.

constexpr auto c_file_separator = L"\\";
constexpr auto c_max_path_length = 1 << 16;

using std::string;
using std::wstring;

namespace ilp_solver
{
    static wstring quote(const wstring& p_string)
    {
        return L"\"" + p_string + L"\"";
    }


    static wstring directory_name(wstring p_filename)
    {
        const auto file_separator_pos = p_filename.rfind(c_file_separator);
        assert(file_separator_pos != std::wstring::npos);
        p_filename.erase(file_separator_pos);
        return p_filename + c_file_separator;
    }


    static wstring executable_dir()
    {
        wchar_t exe_path[c_max_path_length];
        GetModuleFileNameW(NULL, exe_path, c_max_path_length);
        return directory_name(wstring(exe_path));
    }


    static wstring full_executable_name(const string& p_basename)
    {
        return executable_dir() + utf8_to_utf16(p_basename);
    }


    static bool file_exists(const wstring& p_filename)
    {
        WIN32_FIND_DATA find_data;
        const auto handle = FindFirstFileW(p_filename.c_str(), &find_data);

        if (handle == INVALID_HANDLE_VALUE)
            return false;

        FindClose(handle);
        return true;
    }


    static SolverExitCode execute_process(const string& p_executable_basename, const string& p_parameter, int p_wait_milliseconds)
    {
        // get and check executable path
        const auto executable = full_executable_name(p_executable_basename);
        if (!file_exists(executable))
            throw SolverExeException(("Could not find " + p_executable_basename).c_str());

        // prepare command line
        const auto parameter = utf8_to_utf16(p_parameter);
        auto command_line = quote(executable) + L" " + quote(parameter);
        auto non_const_command_line = std::unique_ptr<wchar_t[]>(new wchar_t[command_line.size()+1]); // CreateProcessW needs non-const command line string
        wcscpy_s(non_const_command_line.get(), command_line.size()+1, command_line.c_str());

        // prepare parameters
        STARTUPINFOW startup_info;
        PROCESS_INFORMATION process_info;
        std::memset(&startup_info, 0, sizeof(startup_info));
        std::memset(&process_info, 0, sizeof(process_info));
        startup_info.cb = sizeof(startup_info);

        // start process
        if (!CreateProcessW(executable.c_str(),             // lpApplicationName
                            non_const_command_line.get(),   // lpCommandLine
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
        case SolverExitCode::uncaught_exception_1:
            return "Uncaught exception, likely out of memory (stack buffer overflow Windows 7).";
        case SolverExitCode::uncaught_exception_2:
            return "Uncaught exception, likely out of memory (C++ exception).";
        case SolverExitCode::uncaught_exception_3:
            return "Uncaught exception, likely out of memory (stack buffer overflow Windows 10).";
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
        case SolverExitCode::forced_termination:
            return true;
        default:
            return false;
        }
    }


    static void handle_error(int p_log_level, SolverExitCode p_exit_code)
    {
        if (exit_code_should_be_ignored_silently(p_exit_code))
        {
            if (p_log_level)
                std::cout << exit_code_to_message(p_exit_code) << " Exit Code:" << static_cast<int>(p_exit_code);
        }
        else
            throw std::exception(("External ILP solver: " + exit_code_to_message(p_exit_code)).c_str());
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
        if (exit_code != SolverExitCode::ok)
            handle_error(d_ilp_data.log_level, exit_code);
        else
            communicator.read_solution_data(&d_ilp_solution_data);
    }
}
