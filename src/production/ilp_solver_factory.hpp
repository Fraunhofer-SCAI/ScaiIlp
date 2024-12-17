#pragma once

#include "ilp_solver_interface.hpp"

#include <memory>

// List of all solvers usable from the .dll.
// For new solvers, please follow the declarations below.

namespace ilp_solver
{
namespace impl
{
    extern "C"
#ifdef WITH_CBC
        __declspec(dllexport)
#endif
            ILPSolverInterface* __stdcall create_solver_cbc();


    extern "C"
#ifdef WITH_SCIP
        __declspec(dllexport)
#endif
            ILPSolverInterface* __stdcall create_solver_scip();


    extern "C"
#if defined(WITH_GUROBI) && (_WIN64 == 1)
        __declspec(dllexport)
#endif
            ILPSolverInterface* __stdcall create_solver_gurobi();


    extern "C"
#if defined(WITH_HIGHS) && (_WIN64 == 1)
        __declspec(dllexport)
#endif
            ILPSolverInterface* __stdcall create_solver_highs();


    extern "C"
#ifdef WITH_CBC
        __declspec(dllexport)
#endif
            ILPSolverInterface* __stdcall create_solver_stub(const char* p_executable_basename, bool p_throw_on_all_crashes);


    extern "C" __declspec(dllexport) void __stdcall destroy_solver(ILPSolverInterface* p_solver);
} // namespace impl

struct SolverDeleter
{
    void operator()(ILPSolverInterface* p_solver) { impl::destroy_solver(p_solver); }
};
using ScopedILPSolver = std::unique_ptr<ILPSolverInterface, SolverDeleter>;

inline ScopedILPSolver create_solver_cbc()
{
    return ScopedILPSolver(impl::create_solver_cbc());
}

inline ScopedILPSolver create_solver_scip()
{
    return ScopedILPSolver(impl::create_solver_scip());
}

inline ScopedILPSolver create_solver_gurobi()
{
    return ScopedILPSolver(impl::create_solver_gurobi());
}

inline ScopedILPSolver create_solver_highs()
{
    return ScopedILPSolver(impl::create_solver_highs());
}

inline ScopedILPSolver create_solver_stub(const char* p_executable_basename, bool p_throw_on_all_crashes)
{
    return ScopedILPSolver(impl::create_solver_stub(p_executable_basename, p_throw_on_all_crashes));
}

} // namespace ilp_solver
