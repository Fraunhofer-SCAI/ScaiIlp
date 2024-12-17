#ifndef _ILP_SOLVER_FACTORY_HPP
#define _ILP_SOLVER_FACTORY_HPP

#include "ilp_solver_interface.hpp"

// List of all solvers usable from the .dll.
// For new solvers, please follow the declarations below.

namespace ilp_solver
{
    extern "C"
#if (WITH_CBC == 1)
    __declspec (dllexport)
#endif
    ILPSolverInterface* __stdcall create_solver_cbc();


    extern "C"
#if (WITH_CBC == 1)
    __declspec (dllexport)
#endif
    ILPSolverInterface* __stdcall create_solver_stub(const char* p_executable_basename);


    extern "C"
    __declspec (dllexport)
    void __stdcall destroy_solver(ILPSolverInterface* p_solver);
}

#endif
