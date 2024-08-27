#pragma once

#include "solver_exit_code.hpp"

#include <string>

namespace ilp_solver
{
    // Minimal Variant of the ILPSolverInterface for testing
    // exceptions thrown from such a construct and across DLL boundaries.
    struct ExceptionTester
    {
        virtual void throw_exception(const std::string& p_message) const = 0;
        virtual ~ExceptionTester() = default;
    };

    // Create/Destroy Functionality for the ExceptionTester, as we provide for ILPSolverInterface in ilp_solver_factory.
    extern "C" __declspec(dllexport) ExceptionTester* __stdcall create_exception_tester();
    extern "C" __declspec(dllexport) void             __stdcall destroy_exception_tester(ExceptionTester* p_exception);
    // Tests if stub + ScaiIlpExe work at least for a very easy instance.
    // Useful to check for broken installation, such as antivirus software preventing execution of ScaiIlpExe.
    SolverExitCode stub_tester(const std::string& p_executable_basename);
}
