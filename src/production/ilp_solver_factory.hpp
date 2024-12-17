#ifndef _ILP_SOLVER_FACTORY_HPP
#define _ILP_SOLVER_FACTORY_HPP

#include "ilp_solver_interface.hpp"

namespace ilp_solver
{
    extern "C" ILPSolverInterface* __stdcall create_solver_cbc();
    extern "C" ILPSolverInterface* __stdcall create_solver_stub(const char* p_executable_basename);

    extern "C" void __stdcall destroy_solver(ILPSolverInterface* p_solver);
}

#endif
