#include "ilp_solver_exception.hpp"


namespace ilp_solver
{
    struct ILPSolverExceptionImpl : ILPSolverException
    {
        void throw_exception(const std::string& p_message) const override
        {
            throw std::exception(p_message.c_str());
        }
    };

    extern "C" ILPSolverException* __stdcall create_exception()
    {
        return new ILPSolverExceptionImpl();
    }

    extern "C" void __stdcall destroy_exception(ILPSolverException* p_exception)
    {
        delete p_exception;
    }
}
