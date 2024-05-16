#include "ilp_solver_factory.hpp"

#include "ilp_solver_cbc.hpp"
#include "ilp_solver_gurobi.hpp"
#include "ilp_solver_scip.hpp"
#include "ilp_solver_stub.hpp"


namespace ilp_solver::impl
{
extern "C" ILPSolverInterface* __stdcall create_solver_cbc()
{
#if WITH_CBC == 1
    return new ILPSolverCbc();
#else
    return nullptr;
#endif
}


extern "C" ILPSolverInterface* __stdcall create_solver_gurobi()
{
#if (WITH_GUROBI == 1) && (_WIN64 == 1)
    return new ILPSolverGurobi();
#else
    return nullptr;
#endif
}


extern "C" ILPSolverInterface* __stdcall create_solver_scip()
{
#if WITH_SCIP == 1
    return new ILPSolverSCIP();
#else
    return nullptr;
#endif
}


extern "C" ILPSolverInterface* __stdcall create_solver_stub(const char* p_executable_basename, bool p_throw_on_all_crashes)
{
    return new ILPSolverStub(p_executable_basename, p_throw_on_all_crashes);
}


extern "C" void __stdcall destroy_solver(ILPSolverInterface* p_solver)
{
    delete p_solver;
}
} // namespace ilp_solver
