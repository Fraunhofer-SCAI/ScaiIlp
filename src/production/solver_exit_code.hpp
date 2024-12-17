#ifndef _SOLVER_EXIT_CODE_HPP
#define _SOLVER_EXIT_CODE_HPP

enum class SolverExitCode
{
    ok = 0,

    // Exit codes that we observed when CBC crashed
    uncaught_exception_1 = 3,          // when an exception, namely "Out Of Memory", is thrown from a thread in CBC but not caught
    uncaught_exception_2 = -529697949, // seems to indicate "Out Of Memory (C++ Exception)", see http://www.primegrid.com/forum_thread.php?id=693 or enter this number in Google

    // Our own exit codes, starting from an arbitrary value that is unlikely to be used by Windows for internal codes
    out_of_memory        = 14142,
    command_line_error,
    shared_memory_error,
    model_error,
    solver_error,
    forced_termination
};

#endif
