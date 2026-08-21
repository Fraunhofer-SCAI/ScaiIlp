#include "ilp_solver_factory.hpp"

#include "ilp_solver_cbc.hpp"
#include "ilp_solver_gurobi.hpp"
#include "ilp_solver_highs.hpp"
#include "ilp_solver_scip.hpp"
#include "ilp_solver_stub.hpp"


namespace ilp_solver::impl
{

extern "C" ILPSolverInterface* __stdcall create_solver(const Solver p_solver,
                                      [[maybe_unused]] const char*  p_executable_basename = "ScaiIlpExe.exe",
                                      [[maybe_unused]] bool         p_throw_on_all_crashes = false)
{
    switch (p_solver)
    {
#ifdef WITH_STUB
    case Solver::STUB:   return new ILPSolverStub(p_executable_basename, p_throw_on_all_crashes);
#endif
#ifdef WITH_CBC
    case Solver::CBC:    return new ILPSolverCbc();
#endif
#if defined(WITH_HIGHS) && (_WIN64 == 1)
    case Solver::HIGHS:  return new ILPSolverHighs();
#endif
#ifdef WITH_SCIP
    case Solver::SCIP:   return new ILPSolverSCIP();
#endif
#if defined(WITH_GUROBI) && (_WIN64 == 1)
    case Solver::GUROBI: return new ILPSolverGurobi();
#endif
    }
    return nullptr;
}

extern "C" void __stdcall destroy_solver(ILPSolverInterface* p_solver)
{
    delete p_solver;
}

extern "C" const std::vector<std::pair<Solver, std::string_view>>& __stdcall all_solvers()
{
    static const std::vector<std::pair<Solver, std::string_view>> solver_list =
    {
#ifdef WITH_STUB // If enabled, Stub uses the second solver in this list.
    std::pair{Solver::STUB, "Stub"},
#endif
#ifdef WITH_CBC
    std::pair{Solver::CBC, "CBC"},
#endif
#if defined(WITH_HIGHS) && (_WIN64 == 1)
    std::pair{Solver::HIGHS, "HIGHS"},
#endif
#ifdef WITH_SCIP
    std::pair{Solver::SCIP, "SCIP"},
#endif
#if defined(WITH_GUROBI) && (_WIN64 == 1)
    std::pair{Solver::GUROBI, "GUROBI"},
#endif
    };
    return solver_list;
}

} // namespace ilp_solver
