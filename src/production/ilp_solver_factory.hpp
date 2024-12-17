#pragma once

#include "ilp_solver_interface.hpp"

// List of all solvers usable from the .dll.
// For new solvers, please follow the declarations below.

namespace ilp_solver
{
extern "C"
#if (WITH_CBC == 1)
    __declspec(dllexport)
#endif
        ILPSolverInterface* __stdcall create_solver_cbc();


extern "C"
#if (WITH_SCIP == 1)
    __declspec(dllexport)
#endif
        ILPSolverInterface* __stdcall create_solver_scip();


extern "C"
#if (WITH_GUROBI == 1) && (_WIN64 == 1)
    __declspec(dllexport)
#endif
        ILPSolverInterface* __stdcall create_solver_gurobi();


// Possible values for the Parameter p_crash_mode of create_solver_stub:
#define SCAIILP_SOLVER_STUB_IGNORE_KNOWN_CRASHES 0
#define SCAIILP_SOLVER_STUB_THROW_ON_ALL_CRASHES 1
extern "C"
#if (WITH_CBC == 1)
    __declspec(dllexport)
#endif
        ILPSolverInterface* __stdcall create_solver_stub(const char* p_executable_basename, int p_crash_mode);


extern "C" __declspec(dllexport) void __stdcall destroy_solver(ILPSolverInterface* p_solver);
} // namespace ilp_solver
