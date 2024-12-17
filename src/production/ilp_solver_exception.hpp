#ifndef _ILP_SOLVER_EXCEPTION_HPP
#define _ILP_SOLVER_EXCEPTION_HPP

#include <string>

namespace ilp_solver
{
    // For tests only

    struct ILPSolverException
    {
        virtual void throw_exception(const std::string& p_message) const = 0;
        virtual ~ILPSolverException() {}
    };

    extern "C" __declspec (dllexport) ILPSolverException* __stdcall create_exception();
    extern "C" __declspec (dllexport) void                __stdcall destroy_exception(ILPSolverException* p_exception);
}

#endif
