Table of Contents
=================

1. General Information

    1. About
    2. License
    3. Questions and Answers

2. Building

    1. Building Cbc for ScaiIlp
    2. Optional: Building pthreads-win32 with VS 2022
    3. Building HiGHS with VS 2022
    4. Building SCIP with VS 2022
    5. Building ScaiIlp with VS 2022

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

Currently supported solvers are:
* Cbc
* HiGHS
* SCIP
* Gurobi


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

A: IlpSolverStub is a wrapper for IlpSolverCbc.
   It insulates it from your program.
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

2.1 Building Cbc for ScaiIlp
-----------------------------

1. Download the coinbrew script from https://coin-or.github.io/coinbrew/

2. Follow the instructions described in https://coin-or.github.io/user_introduction#windows-1 to compile it to your specifications.

3. We require the configuration options
    * --enable-shared
    * ADD_CXXFLAGS=-D_ITERATOR_DEBUG_LEVEL=0

4. If Cbc should support multi-threading via pthreads (see section 2.2), you also need to provide the configuration options
    * --enable-cbc-parallel
    * --with-pthreadsw32-lib=path_to_pthreads_lib
    * --with-pthreadsw32-incdir=path_to_pthreads_include

2.2 Optional: Building pthreads-win32 with VS 2022 (only if Cbc should support multi-threading)
-----------------------------------------------------------------------------------------------

1. Download pthreads-win32 from the [Pthreads-Win32 project](https://sourceware.org/pthreads-win32/)
    * Download: ftp://sourceware.org/pub/pthreads-win32/

2. Open pthread.dsw with VS 2022 and let it upgrade the project.

3. When you want to compile for 64 bit platforms:
    * Choose "Build" -> "Configuration Manager".
    * In the "Active solution platform" dropdown menu, select "New" and choose "x64" as new platform.
    * Make sure that "Copy settings from: x86" is chosen and "Create new project platforms" is checked.

4. Choose "Build" -> "Configuration Manager". Choose the "Active solution platform" depending on your needs:
    * Choose "x86" when compiling for 32 bit platforms
    * Choose "x64" when compiling for 64 bit platforms

5. Right-click onto the project "pthread" in the Solution Explorer and choose
    * "Properties" -> "Configuration Properties".
    * Select "All Configurations" and "All Platforms" and use the following settings:
        * General / Output Directory:                `$(SolutionDir)$(PlatformTarget)-$(PlatformToolset)-$(Configuration)\`
        * General / Intermediate Directory:          `$(PlatformTarget)-$(PlatformToolset)-$(Configuration)\`
        * General / Windows SDK Version:             Your current SDK
        * General / Platform Toolset:                `Visual Studio 2022 (v143)`
        * C/C++ / General             / Debug Information Format:       `Program Database (/Zi)`
        * C/C++ / Preprocessor        / Preprocessor Definitions:       prepend `_ITERATOR_DEBUG_LEVEL=0;_TIMESPEC_DEFINED;`
        * C/C++ / Precompiled Headers / Precompiled Header Output File: `$(IntDir)pthread.pch`
        * C/C++ / Output Files        / ASM List Location:              `$(IntDir)`
        * C/C++ / Output Files        / Object File Name:               `$(IntDir)`
        * C/C++ / Output Files        / Program Database File Name:     `$(IntDir)`
        * Linker / General   / Output File:         `$(OutDir)$(TargetName)$(TargetExt)`
        * Linker / Debugging / Generate Debug Info: `Generate Debug Information (/DEBUG)`
        * Linker / Advanced  / Import Library:      `$(OutDir)$(TargetName).lib`

    * For faster compilation, you can additionally use the following settings (set none or both):
        * C/C++   / General         / Multi-processor Compilation:  `Yes (/MP)`
        * C/C++   / Code Generation / Enable Minimal Rebuild:       `No (/Gm-)`

8. In the Solution Explorer
    * Find the filter "Resource Files"
    * Right-click onto "version.rc"
    * Choose "Properties".
    * Select "All Configurations"
    * Resources / General / Preprocessor Definitions:
        * prepend `PTW32_ARCHx86;` when compiling for 32 bit platforms
        * or      `PTW32_ARCHx64;` (without double quotes) otherwise.

9. Find the file pthreads.h in project "pthread" -> "Header Files"
    * At the top of the file, insert the line `#define _TIMESPEC_DEFINED`

2.3 Building HiGHS with VS 2022
-------------------------------

1. Download the HiGHS source code from https://github.com/ERGO-Code/HiGHS.

2. Uncomment the line `add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)` in CMakeLists.txt.

3. Open CMakeLists.txt (the file in the root folder) in VS2022 with File -> Open -> CMake.

4. Build and install all desired configurations.

2.4 Building SCIP with VS 2022
------------------------------

1. To obtain SCIP, visit https://scip.zib.de/index.php#download
   and download the SCIP Optimization Suite.

2. Open CMakeLists.txt (the file in the root folder) in VS2022 with File -> Open -> CMake.

3. You may need to overwrite some compiler flags to compile the Release builds.
   In the files ./scip/CMakeLists.txt and ./soplex/CMakeLists.txt in lines 3 and 4 respectively,

   ```
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_RELEASE}")
       -> set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE}")

   set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS_RELEASE}")
       -> set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE}")
   ```

4. You may want to set different build/install directories.

5. Compile the needed configurations [32|64] bit in [Release | Debug] mode.

6. This version of SCIP will run only single-threaded.
   To use SCIP multithreaded you will need to compile it with the Ubiquity Generator (UG) framework.
   The UG-framework does only provide makefiles,
   so doing this on Windows is not easy and we can not provide a guideline for it.

2.5 Building ScaiIlp with VS 2022
---------------------------------

1. Ensure that you have built Cbc as described above.

2. Ensure that you have built Boost. When building Boost, you need to set the parameters
    * `define=_ITERATOR_DEBUG_LEVEL=0`
    * `define=BOOST_TEST_NO_MAIN`

3. Specify the location of Cbc by opening the properties.props file
   and setting the User Macros `WITH_OSI` and `WITH_CBC` to 'true', and `COIN_DIR` to the correct path (if your paths follow our examples).
   If your paths do not follow our examples, you may want to manually edit the include and linker directories or the properties.props outside of VS.

4. [OPTIONAL] If you want to support multithreading,
   specify the root-location of pthread with the User Macro `PTHREAD_DIR` in the properties.props file.
   Then set `PTHREAD_LIB_PATH`, such that it points to the folder containing the appropriate version of pthread.dll.
   Usually this will be `$(PTHREAD_DIR)\$(PlatformTarget)-v$(PlatformToolsetVersion)-$(Configuration)`.
   If your structure does not follow this, you may want to manually edit the corresponding CustomBuild setting in ScaiIlpDll.vcxproj.

5. [OPTIONAL] If you want to support HiGHS,
   specify the location of HiGHS by opening the properties.props file
   and setting the User Macro `WITH_HIGHS` to 'true' and `HIGHS_DIR` to the correct path (if your paths follow our examples).
   If your paths do not follow our examples, you may want to manually edit the include and linker directories or the properties.props outside of VS.

5. [OPTIONAL] If you want to support SCIP,
   specify the location of SCIP by opening the properties.props file
   and setting the User Macro `WITH_SCIP` to 'true' and `SCIP_DIR` to the correct path (if your paths follow our examples).
   If your paths do not follow our examples, you may want to manually edit the include and linker directories or the properties.props outside of VS.

6. [OPTIONAL] If you want to support Gurobi,
   specify the location of Gurobi by opening the properties.props file
   and setting the User Macro `WITH_GUROBI` to 'true' and `GUROBI_DIR` to the root directory of your Gurobi Installation.
   Note that current versions of Gurobi only support 64-bit compilation,
   and that you need a valid Gurobi license to run ScaiILP with Gurobi.

7. Specify the location of Boost by opening the properties.props file
   and setting the User Macros `BOOST_VERSION` and `BOOST_DIR` (if your paths follow our examples)
   or by setting the `BOOST_INCLUDE_PATH` and `BOOST_LIB_PATH` manually to the correct paths on your system outside of VS.

8. Build ScaiIlpDll, ScaiIlpExe, and UnitTest.


3 Code Structure
================

3.1 Projects in Visual Studio
-----------------------------

The Visual Studio Solution (.sln) contains three projects:

* ScaiIlpDll creates ScaiIlpDll.dll
    * ScaiIlpDll.dll contains the Cbc solver and a stub to communicate with ScaiIlpExe.exe.
      Optionally, it links dynamically to the SCIP solver and the Gurobi solver.
    * It can be linked dynamically into other programs (which may require the dynamic libraries of other included solvers, too).
    * The required dynamic libraries are automatically copied to the output folder when building ScaiIlpDll.

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
* Create your objects via create_solver_xxx() from ilp_solver_factory.hpp.
* To destroy the solver later, you MUST call destroy_solver() instead of deleting the pointer
  yourself.

### 3.2.2 Static linking

Alternatively, you may include ilp_solver_xxx.cpp and all its dependencies in your project.
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
        |   |                   Base class for all solvers where that is useful.
        |   |
        |   |-> ILPSolverStub:  Final. Solve in a separate process.
        |                       solve_impl() writes the ILPData to shared memory and calls an external solver.
        |                       The external solver writes the result (in form of ILPSolutionData)
        |                       back to the shared memory.
        |                       The solution getter methods of ILPSolverStub simply query ILPSolutionData.
        |
        |-> ILPSolverHighs:     Final. To use HiGHS.
        |                       Implements the solver specific methods for the HiGHS solver.
        |
        |-> ILPSolverSCIP:      Final. To use SCIP.
        |                       Implements the solver specific methods for the SCIP solver.
        |
        |-> ILPSolverGurobi:    Final. To use Gurobi.
                                Implements the solver specific methods for the Gurobi solver.


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
```
    extern "C" __declspec (dllexport) ILPSolverInterface* __stdcall create_solver_xyz(parameters);
```
in ilp_solver_factory.hpp and define it in ilp_solver_factory.cpp, respectively.
