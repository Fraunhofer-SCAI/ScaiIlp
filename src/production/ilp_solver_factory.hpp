#pragma once

#include "ilp_solver_interface.hpp"

#include <memory>

// List of all solvers usable from the .dll.
// For new solvers, please follow the declarations below.

namespace ilp_solver
{
enum class Solver { STUB, CBC, HIGHS, SCIP, GUROBI };

namespace impl
{
    extern "C" __declspec(dllexport) ILPSolverInterface* __stdcall create_solver(
        const Solver p_solver, const char* p_executable_basename, bool p_throw_on_all_crashes);

    extern "C" __declspec(dllexport) void __stdcall destroy_solver(ILPSolverInterface* p_solver);

    extern "C" __declspec(dllexport) const std::vector<std::pair<Solver, std::string_view>>& __stdcall all_solvers();
} // namespace impl

struct SolverDeleter
{
    void operator()(ILPSolverInterface* p_solver) { impl::destroy_solver(p_solver); }
};
using ScopedILPSolver = std::unique_ptr<ILPSolverInterface, SolverDeleter>;

// Returns nullptr if selected solver is not available in ScaiILP.
inline ScopedILPSolver create_solver(const Solver p_solver, [[maybe_unused]] const char* p_executable_basename = "ScaiIlpExe.exe",
                                     [[maybe_unused]] bool p_throw_on_all_crashes = false)
{
    return ScopedILPSolver(impl::create_solver(p_solver, p_executable_basename, p_throw_on_all_crashes));
}

} // namespace ilp_solver
