Table of Contents
=================

1. General Information

    1. About
    2. License
    3. Questions and Answers

2. Building

    1. Building Cbc with VS 2017
    2. Optional: Building pthreads-win32 with VS 2017
    3. Building ScaiIlp with VS 2017

3. Code Structure

    1. Projects in Visual Studio
    2. Usage
    3. Class Hierarchy
    4. Adding a New Solver


1 General Information
=====================

1.1 About
---------

ScaiIlp can provide an interface to different ILP solvers.
Currently, the only supported solver is Cbc, see section 2.1.


1.2 License
-----------

ScaiIlp is licensed under the terms of the Eclipse Public License (EPL), version 1.0

See https://opensource.org/licenses/EPL-1.0 or license.txt


1.3 Questions and Answers
-------------------------

### Q: What is the purpose of ScaiIlp?

A: There are two.

1. Provide a unified and simplified interface for different ILP solvers.
2. Allow dynamic linking without having to modify the sources/makefiles of solvers that do not
   support this natively.

### Q: What about Osi? Why another interface?

A: On the one hand, Osi is quite a complex interface. As it spreads over several files with several
   classes, it is difficult to use as the interface of a DLL.
   On the other hand, some functions that we needed are missing in Osi.

### Q: When should I use IlpSolverStub and ScaiIlpExe?

A: Having the solver in a separate process insulates it from your program.
   If the solver crashes, your program can survive this.
   On any crashes we know of, IlpSolverStub does silently the same as if the solver just found no
   solution.
   On unknown crashes and unknown problems, IlpSolverStub throws an exception, which can be caught
   in your code.

### Q: When should I use IlpSolverCbc directly?

A: If you don't experience solver crashes, you can avoid some overhead by using IlpSolverCbc
   directly.


2 Building
==========

2.1 Building Cbc with VS 2017
-----------------------------

1. Download Cbc from the [COIN project](https://www.coin-or.org/).
    * Download: https://www.coin-or.org/download/source/Cbc/
    * via SVN:  svn co https://projects.coin-or.org/svn/Cbc/stable/2.8 coin-Cbc
    * Wiki:     https://projects.coin-or.org/Cbc/wiki

2. Copy the directory Cbc\MSVisualStudio\v10 to Cbc\MSVisualStudio\v141 and open Cbc\MSVisualStudio\v141\Cbc.sln with VS 2017

3. Let VS update the projects (Windows SDK Version 8.1, PlatForm Toolset v141)

4. Right-click onto "Solution" in the Solution Explorer and choose "Properties". Then use the following settings:
    * Common Properties / Project Dependencies:
        * Choose "libOsiCbc" from "Projects" and check the following 2 projects:
            * libCbc
            * libOsi
        * Choose "libCbc" from "Projects" and check the following 5 projects:
            * libCgl
            * libClp
            * libCoinUtils
            * libOsi
            * libOsiClp

5. Choose "Build" -> "Configuration Manager". Choose the "Active solution platform" depending on your needs:
    * Choose "Win32" when compiling for 32 bit platforms
    * Choose "x64" when compiling for 64 bit platforms

6. In the Solution Explorer, mark the following 7 projects:
    * libCbc
    * libCgl
    * libClp
    * libCoinUtils
    * libOsi
    * libOsiCbc
    * libOsiClp

7. Right-click onto the marked projects and choose "Properties" -> "Configuration Properties".
    * Select "All Configurations" and "All Platforms" and use the following settings:
        * General / Output Directory:                $(SolutionDir)$(Platform)-$(PlatformToolset)-$(Configuration)\
        * General / Intermediate Directory:          $(Platform)-$(PlatformToolset)-$(Configuration)\
        * General / Windows SDK Version:             8.1
        * General / Platform Toolset:                Visual Studio 2017 (v141)
        * C/C++   / Preprocessor / Preprocessor Definitions:   prepend "_ITERATOR_DEBUG_LEVEL=0;" (without double quotes)
        * C/C++   / Output Files / Program Database File Name: $(OutDir)$(TargetName).pdb

    * For faster compilation, you can additionally use the following settings (set none or both):
        * C/C++   / General         / Multi-processor Compilation:  Yes (/MP)
        * C/C++   / Code Generation / Enable Minimal Rebuild:       No (/Gm-)

8. If you want to support multithreading: Build pthreads-win32 as described in section 2.2 Otherwise proceed as in step 9.
    * Right-click onto the project "libCbc" in the Solution Explorer and choose
    * "Properties" -> "Configuration Properties".
    * Select "All Configurations" and "All Platforms" and use the following settings:
        * C/C++     / General      / Additional Include Directories: prepend "$(PTHREAD_DIR);" (without double quotes)
        * C/C++     / Preprocessor / Preprocessor Definitions:       prepend "CBC_THREAD;" (without double quotes)
        * Librarian / General      / Additional Dependencies:        pthread.lib
        * Librarian / General      / Additional Library Directories: $(PTHREAD_DIR)\$(Platform)-$(PlatformToolset)-$(Configuration)

9. Choose "Build" -> "Batch-Build" and select the following projects and configurations:
    * for 32 bit:
        * libOsiCbc  Debug    Win32
        * libOsiCbc  Release  Win32
    * for 64 bit:
        * libOsiCbc  Debug    x64
        * libOsiCbc  Release  x64


2.2 Optional: Building pthreads-win32 with VS 2017 (only if Cbc should support multi-threading)
-----------------------------------------------------------------------------------------------

1. Download pthreads-win32 from the [Pthreads-Win32 project](https://sourceware.org/pthreads-win32/)
    * Download: ftp://sourceware.org/pub/pthreads-win32/

2. Open pthread.dsw with VS 2017 and let it upgrade the project.

3. When you want to compile for 32 bit platforms:
    * Choose "Build" -> "Configuration Manager".
    * In the "Active solution platform" dropdown menu, select "Edit" and rename "x86" to "Win32".

   When you want to compile for 64 bit platforms:
    * Choose "Build" -> "Configuration Manager".
    * In the "Active solution platform" dropdown menu, select "New" and choose "x64" as new platform.
    * Make sure that "Copy settings from: Win32" is chosen and "Create new project platforms" is checked.

4. Choose "Build" -> "Configuration Manager". Choose the "Active solution platform" depending on your needs:
    * Choose "Win32" when compiling for 32 bit platforms
    * Choose "x64" when compiling for 64 bit platforms

5. Right-click onto the project "pthread" in the Solution Explorer and choose
    * "Properties" -> "Configuration Properties".
    * Select "All Configurations" and "All Platforms" and use the following settings:
        * General / Output Directory:                $(SolutionDir)$(Platform)-$(PlatformToolset)-$(Configuration)\
        * General / Intermediate Directory:          $(Platform)-$(PlatformToolset)-$(Configuration)\
        * General / Windows SDK Version:             8.1
        * General / Platform Toolset:                Visual Studio 2017 (v141)
        * C/C++ / General             / Debug Information Output:       Program Database (/Zi)
        * C/C++ / Preprocessor        / Preprocessor Definitions:       prepend "_ITERATOR_DEBUG_LEVEL=0;_TIMESPEC_DEFINED;" (without double quotes)
        * C/C++ / Precompiled Headers / Precompiled Header Output File: $(IntDir)pthread.pch
        * C/C++ / Output Files        / ASM List Location:              $(IntDir)
        * C/C++ / Output Files        / Object File Name:               $(IntDir)
        * C/C++ / Output Files        / Program Database File Name:     $(IntDir)
        * Linker / General   / Output File:         $(OutDir)$(TargetName)$(TargetExt)
        * Linker / Debugging / Generate Debug Info: Generate Debug Information (/DEBUG)
        * Linker / Advanced  / Import Library:      $(OutDir)$(TargetName).lib

    * For faster compilation, you can additionally use the following settings (set none or both):
        * C/C++   / General         / Multi-processor Compilation:  Yes (/MP)
        * C/C++   / Code Generation / Enable Minimal Rebuild:       No (/Gm-)

8. In the Solution Explorer
    * Find the filter "Resource Files"
    * Right-click onto "version.rc"
    * Choose "Properties".
    * Select "All Configurations"
    * Resources / General / Preprocessor Definitions:
        * prepend "PTW32_ARCHx86;" (without double quotes) when compiling for 32 bit platforms
        * or      "PTW32_ARCHx64;" (without double quotes) otherwise.

9. Find the file pthreads.h in project "pthread" -> "Header Files"
    * At the top of the file, insert the line "#define _TIMESPEC_DEFINED" (without double quotes)


2.3 Building ScaiIlp with VS 2017
---------------------------------

1. Ensure that you have built Cbc as described above.

2. Ensure that you have built Boost. When building Boost, you need to set the parameters
    * "define=_ITERATOR_DEBUG_LEVEL=0" (without double quotes)
    * "define=BOOST_TEST_NO_MAIN"      (without double quotes)

3. Specify the location of Cbc by opening the properties.props file
   and setting the User Macro COIN_DIR (if your paths follow our examples)
   or by setting the COIN_LIB_PATH and COIN_INCLUDE_PATHS macros to the correct paths on your system.

4. Activate CBC by opening the properties.props file and setting the preprocessor definitions "WITH_CBC=1" and "WITH_OSI=1".
   You can find the preprocessor definitions under C/C++ -> Preprocessor -> Preprocessor Definitions.
   CBC should be activated by default.

5. If you want to support multithreading,
   specify the location of pthread by setting the environment variable PTHREAD_DIR,
   whereby PTHREAD_DIR has to contain the folders "Win32-v141-Release", "Win32-v141-Debug", "x64-v141-Release" and "x64-v141-Debug",
   each containing the appropriate version of pthread.dll. Otherwise proceed with step 5.

6. Specify the location of Boost by opening the properties.props file
   and setting the User Macros BOOST_VERSION and BOOST_DIR (if your paths follow our examples)
   or by setting the BOOST_INCLUDE_PATH and BOOST_LIB_PATH manually to the correct paths on your system.

7. Build ScaiIlpDll, ScaiIlpExe, and UnitTest.


3 Code Structure
================

3.1 Projects in Visual Studio
-----------------------------

The Visual Studio Solution (.sln) contains three projects:

* ScaiIlpDll creates ScaiIlpDll.dll
    * ScaiIlpDll.dll contains the Cbc solver and a stub to communicate with ScaiIlpExe.exe
    * It can be linked dynamically into other programs.

* ScaiIlpExe creates ScaiIlpExe.exe
    * ScaiIlpExe.exe links ScaiIlpDll.dll dynamically to provide the Cbc solver.
    * ScaiIlpExe.exe can be started in a separate process and communicates via shared memory.

* UnitTest demonstrates usage for both of above projects.


3.2 Usage
---------

The published solver interface is ILPSolverInterface.
Include ilp_solver_interface.hpp.

### 3.2.1 Use as a DLL

The recommended way to use ScaiIlp is to use it as a DLL (dynamic linking)

* Link against ScaiIlpDll.dll.
* Include ilp_solver_factory.hpp.
* Create your objects via create_solver_cbc() from ilp_solver_factory.hpp.
* To destroy the solver later, you MUST call destroy_solver() instead of deleting the pointer
  yourself.

### 3.2.2 Static linking

Alternatively, you may include ilp_solver_cbc.cpp and all its dependencies in your project.
This way, your code gets statically linked with a part of ScaiIlp.

### 3.2.3 ScaiIlpExe.exe

To use ScaiIlpExe.exe, there is a class ILPSolverStub.
IlpSolverStub can be used like IlpSolverCbc either as described in 3.2.1 or as described in 3.2.2.
The constructor of IlpSolverStub and create_solver_stub() expect the base name of a solver
executable (in the same directory, should be ScaiIlpExe.exe, unless you rename it).


3.3 Class Hierarchy
-------------------

    ILPSolverInterface:         Published interface
    |
    |-> ILPSolverImpl:          Auxiliary base class to simplify implementation of any specific solver.
        |                       Implements some methods of ILPSolverInterface
        |                       by calling a smaller number of newly introduced private virtual methods.
        |
        |-> ILPSolverOsiModel:  Base class for solvers whose modeling functionality is exposed via
        |   |                   the OsiSolverInterface, i.e. have a partial Osi interface.
        |   |                   Implements all methods they share.
        |   |
        |   |-> ILPSolverCbc:   Final. To use CBC.
        |   |                   Implements the remaining, solver specific methods for the CBC solver.
        |   |
        |   |-> ILPSolverOsi:   Final. Class for solvers who have a complete Osi interface.
        |                       Implements the remaining, solver specific methods.
        |                       Currently, some parameter-setting functions have empty implementations
        |                       because the OsiSolverInterface does not provide this functionality.
        |
        |-> ILPSolverCollect:   Implements all input related methods by storing the data in ILPData.
            |                   Base class for all solvers where that is useful.
            |
            |-> ILPSolverStub:  Final. Solve in a separate process.
                                solve_impl() writes the ILPData to shared memory and calls an external solver.
                                The external solver writes the result (in form of ILPSolutionData)
                                back to the shared memory.
                                The solution getter methods of ILPSolverStub simply query ILPSolutionData.


3.4 Adding a New Solver
-----------------------

When you want to support a new solver, you must ask yourself at which level you want to hook into
the class hierarchy.

1. If you want to communicate with the solver via the OsiSolverInterface, can use the ILPSolverOsi class.
   The constructor takes any valid OsiSolverInterface*.

   Note, however, that the OsiSolverInterface does not provide all the functionality that is
   exposed by ILPSolverInterface. If your solver provides a non-Osi interface, you might prefer
   the latter. If your solver has ways to partially bypass Osi and add the missing functionality,
   you should derive from IlpSolverOsi and override the corresponding functions.

2. If your solver is based on an LP-Solver it communicates with via the OsiSolverInterface and if
   your solver obtains its model via this LP-solver, then you should derive from ILPSolverOsiModel
   like Cbc does.

3. If you don't use Osi at all, you should derive from ILPSolverImpl.

   Most likely you don't want to derive from IlpSolverInterface directly.

If you want your solver to be accessible via the DLL, then you must declare a function

    extern "C" __declspec (dllexport) ILPSolverInterface* __stdcall create_solver_xyz(parameters);

in ilp_solver_factory.hpp and define it in ilp_solver_factory.cpp, respectively.
