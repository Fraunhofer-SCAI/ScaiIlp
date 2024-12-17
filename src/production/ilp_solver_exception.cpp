#include "ilp_solver_exception.hpp"

#include <stdexcept>

namespace ilp_solver
{
    struct ExceptionTesterImpl : ExceptionTester
    {
        void throw_exception(const std::string& p_message) const override
        {
            throw std::runtime_error(p_message);
        }
    };


    extern "C" ExceptionTester* __stdcall create_exception_tester()
    {
        return new ExceptionTesterImpl();
    }


    extern "C" void __stdcall destroy_exception_tester(ExceptionTester* p_exception)
    {
        delete p_exception;
    }
}
