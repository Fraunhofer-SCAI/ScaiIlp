#pragma once

#include <string>

namespace ilp_solver
{
    // Minimal Variant of the ILPSolverInterface for testing
    // exceptions thrown from such a construct and across DLL boundaries.
    struct ExceptionTester
    {
        virtual void throw_exception(const std::string& p_message) const = 0;
        virtual ~ExceptionTester() {}
    };

    // Create/Destroy Functionality for the ExceptionTester, as we provide for ILPSolverInterface in ilp_solver_factory.
    extern "C" __declspec(dllexport) ExceptionTester* __stdcall create_exception_tester();
    extern "C" __declspec(dllexport) void             __stdcall destroy_exception_tester(ExceptionTester* p_exception);
}
