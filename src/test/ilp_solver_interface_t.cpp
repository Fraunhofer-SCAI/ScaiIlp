#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include <array>
#include <algorithm>
#include <iostream>
#include <string_view>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#define NOMINMAX
#include <windows.h>    // for GetTickCount

using std::cout;
using std::endl;
using std::string;
using std::vector;

const auto c_eps = 0.0001;
const auto c_num_performance_test_repetitions = 10000;

const bool LOGGING = true;

namespace ilp_solver
{
    static int round(double x)
    {
        return (int) (x+0.5);
    }


    static double rand_double()
    {
        return -0.5 + (1.0*rand())/RAND_MAX;
    }


    static std::pair<int, int> generate_random_problem(ILPSolverInterface* p_solver, int p_num_variables, int p_num_constraints)
    {
        srand(3);
        static constexpr double variable_scaling = 10.0;
        static const double   constraint_scaling = p_num_variables * variable_scaling;

        const auto start_time = GetTickCount();

        for (auto j = 0; j < p_num_variables; ++j)
            p_solver->add_variable_integer(rand_double(), variable_scaling*rand_double(), variable_scaling*(1.0 + rand_double()));

        const auto middle_time = GetTickCount();

        std::vector<double> constraint_vector(p_num_variables);

        for (auto i = 0; i < p_num_constraints; ++i)
        {
            std::generate(std::begin(constraint_vector), std::end(constraint_vector), []() { return rand_double(); });
            p_solver->add_constraint(constraint_vector, constraint_scaling*rand_double(), constraint_scaling*(1.0 + rand_double()));
        }
        const auto end_time = GetTickCount();
        return {middle_time - start_time, end_time - middle_time};
    }


    using TestFunction    = void(*)(ILPSolverInterface*);
    using FactoryFunction = ILPSolverInterface* (__stdcall *)(void);
    using StubFunction    = ILPSolverInterface* (__stdcall *)(const char*);

    void execute_test_and_destroy_solver(ILPSolverInterface* p_solver, TestFunction p_test)
    {
        try
        {
            p_test(p_solver);
        }
        catch (...)
        {
            destroy_solver(p_solver);
            throw;
        }
        destroy_solver(p_solver);
    }


    void test_sorting(ILPSolverInterface* p_solver)
    {
        std::stringstream logging;

        std::vector<int> numbers{ 62, 20, 4, 49, 97, 73, 35, 51, 18, 86};
        const auto num_vars = (int) numbers.size();

        // xi - target position of numbers[i]
        //
        // min x0 + ... + x9
        // s.t. xk - xl >= 0.1 for every (k,l) for which numbers[k] > numbers[l] (Note: If we set the rhs to 1.0, then the integrality constraint is superfluous.)
        //      xi >= 0 integral

        // Add variables
        for (auto i = 0; i < num_vars; ++i)
            p_solver->add_variable_integer(1, 0, std::numeric_limits<int>::max(), "x" + std::to_string(i));

        // Add constraints
        logging << "Initial array: ";

        vector<double> values{1., -1.};
        vector<int> indices{0, 0};
        for (auto i = 0; i < num_vars; ++i)
        {
            for (auto j = i+1; j < num_vars; ++j)
            {
                // Reorder i and j (naming them k and l) s.t. numbers[k] > numbers[l]
                auto k = i;
                auto l = j;
                if (numbers[k] < numbers[l])
                    std::swap(k, l);

                indices[0] = k;
                indices[1] = l;

                p_solver->add_constraint_lower(indices, values, 1.0, "x" + std::to_string(k) + ">x" + std::to_string(l));
            }
            logging << numbers[i] << " ";
        }
        logging << endl << endl;

        p_solver->minimize();

        const auto obj_value = p_solver->get_objective();
        const auto permutation = p_solver->get_solution();
        const auto status = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        logging << "Solution is " << (optimal ? "" : "not ") << "optimal" << endl;
        BOOST_REQUIRE(optimal);

        // Check correctness of objective
        const auto expected_obj_value = num_vars*(num_vars-1)/2;
        BOOST_REQUIRE_CLOSE(obj_value, expected_obj_value, c_eps);  // objective should be 0+1+...+(num_numbers-1)

        // Check correctness of solution
        auto sorted = vector<int>(num_vars, INT_MIN);       // sort according solution

        logging << endl << "Resulting permutation: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            const auto pos = round(permutation[i]);

            logging << pos << " ";

            BOOST_REQUIRE_CLOSE(pos, permutation[i], c_eps);    // solution must be integral
            sorted[pos] = numbers[i];
        }
        logging << endl;

        logging << "Sorted array: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            logging << sorted[i] << " ";

            BOOST_REQUIRE_NE(sorted[i], INT_MIN);                   // solution must be a permutation
            if (i > 0)
                BOOST_REQUIRE_LT(sorted[i-1], sorted[i]);            // solution must sort
        }
        logging << endl;

        if (LOGGING)
            cout << logging.str();
    }


    void test_linear_programming(ILPSolverInterface* p_solver)
    {
        std::stringstream logging;

        const auto num_vars = 5;
        const auto num_dirs = num_vars;
        const auto constraint_shift = 10.0;

        // expected solution
        double x0[num_vars] = { 2.72, 42.0, -1.41, 3.14, -1.62 };

        // constraint directions
        double a[num_dirs][num_vars] = {{ 1.24, -3.47, 8.32, 4.78, -5.34 },
                                        { -7.23, 4.90, -3.21, 0.39, 9.45 },
                                        { 2.40, 9.38, -6.67, -6.43, 5.38 },
                                        { -4.79, 1.47, 6.47, 4.30, -8.39 },
                                        { 8.32, -7.20, 4.96, -9.41, 3.64 }};

        double scalar[num_dirs] = { 7, 2, 5, 6, 3 };

        // objective c = sum_j scalar[j]*a[j] (conical combination)
        double c[num_vars];

        logging << "Objective: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            c[i] = 0.0;
            for (auto j = 0; j < num_dirs; ++j)
                c[i] += scalar[j]*a[j][i];

            logging << c[i] << " ";
        }
        logging << endl;

        // optimal objective
        auto obj0 = 0.0;
        for (auto i = 0; i < num_vars; ++i)
            obj0 += c[i]*x0[i];

        // right hand sides b[j] = a[j]*expected_solution (scalar product)
        double b[num_dirs];
        for (auto j = 0; j < num_dirs; ++j)
        {
            b[j] = 0.0;
            for (auto i = 0; i < num_vars; ++i)
                b[j] += a[j][i]*x0[i];
        }

        // max c*(x0,...,x4)
        // s.t. b[j] - constraint_shift <= a[j]*(x0,...,x4) <= b[j] for all j

        // Add variables
        for (auto i = 0; i < num_vars; ++i)
            p_solver->add_variable_continuous(c[i], std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), "x" + std::to_string(i));

        // Add constraints
        logging << "Constraints:" << endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            const auto values = vector<double>(a[j], a[j] + num_vars);

            auto j_s{ std::to_string(j) };
            if (j % 2)
            {
                p_solver->add_constraint_lower(values, b[j] - constraint_shift, "x*dir" + j_s + " >= b" + j_s + " - 10");
                p_solver->add_constraint_upper(values, b[j],                    "x*dir" + j_s + " <= b" + j_s);
            }
            else
                p_solver->add_constraint(values, b[j] - constraint_shift, b[j], "b" + j_s + " - 10 <= x*dir" + j_s + " <= b" + j_s);

            logging << b[j] - constraint_shift << " <= ";
            for (auto i = 0; i < num_vars; ++i)
                logging << (i == 0 ? "" : " + ") << a[j][i] << "*x" << i;
            logging << " <= " << b[j] << endl;
        }
        logging << endl;

        p_solver->maximize();

        const auto obj = p_solver->get_objective();
        const auto x = p_solver->get_solution();
        const auto status = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        BOOST_REQUIRE(optimal);
        logging << "The solution is " << (optimal ? "" : "not ") << "optimal." << endl;

        // Check correctness of objective
        auto obj_cmp = 0.0;
        for (auto i = 0; i < num_vars; ++i)
            obj_cmp += c[i]*x[i];
        BOOST_REQUIRE_CLOSE(obj, obj_cmp, c_eps);   // objective should fit to the solution

        logging << endl;
        logging << "Expected objective: " << obj0 << endl;
        logging << "Resulting objective: " << obj << endl;

        BOOST_REQUIRE_CLOSE(obj, obj0,  c_eps);      // objective should equal the optimal objective

        // Check correctness of solution
        logging << endl << "Constraint values:" << endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            auto constraint_value = 0.0;
            for (auto i = 0; i < num_vars; ++i)
                constraint_value += a[j][i]*x[i];

            logging << constraint_value << " (must be in [" << b[j] -constraint_shift << "," << b[j] << "), expected " << b[j] << endl;

            BOOST_REQUIRE_GE(constraint_value, b[j] - constraint_shift - c_eps);  // solution obeys lower bound of the j'th constraint
            BOOST_REQUIRE_LE(constraint_value, b[j] + c_eps);                     // solution obeys upper bound of the j'th constraint
            BOOST_REQUIRE_GE(constraint_value, b[j] - c_eps);                     // upper bound of the j'th constraint is tight
        }

        logging << endl << "Expected solution: ";
        for (auto i = 0; i < num_vars; ++i)
            logging << x0[i] << " ";
        logging << endl;

        logging << "Resulting solution: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            logging << x[i] << " ";
            BOOST_REQUIRE_CLOSE(x[i], x0[i], c_eps);
        }
        logging << endl;

        if (LOGGING)
            cout << logging.str();
    }


    // Tests only for file creation and non-null-size.
    // Correctness of file can only be tested if we implement reading from mps, too.
    void test_mps_output(ILPSolverInterface* p_solver, const std::string& p_path)
    {
        // Disable output for problem construction and solve.
        std::cout.setstate(std::ios_base::failbit);
        test_linear_programming(p_solver);
        std::cout.clear();

        p_solver->print_mps_file(p_path);

        std::filesystem::path path{ p_path };
        BOOST_REQUIRE(std::filesystem::is_regular_file(path));

        auto size{ std::filesystem::file_size(path) };
        BOOST_REQUIRE(std::filesystem::file_size(path) > 0);

        if (LOGGING)
        {
            cout << "Successfully wrote mps-File to " << std::filesystem::absolute(path).generic_u8string() << ".\n";
            cout << "\tFilesize is " << size << " Byte." << std::endl;
        }
    }


    void test_performance(ILPSolverInterface* p_solver)
    {
        // max x+y, -1 <= x, y <= 1
        p_solver->add_variable_continuous(1, -1, 1);
        p_solver->add_variable_continuous(1, -1, 1);

        // x+2y <= 2
        vector<double> values_1{1, 2};
        p_solver->add_constraint_upper(values_1, 2);

        // 2x+y <= 2
        vector<double> values_2{2,1};
        p_solver->add_constraint_upper(values_2, 2);

        vector<double> expected_solution{2./3., 2./3.};

        const auto start_time = GetTickCount();
        for (auto i = 1; i <= c_num_performance_test_repetitions; ++i)
        {
            p_solver->reset_solution();
            BOOST_REQUIRE(p_solver->get_status() == SolutionStatus::NO_SOLUTION);
            BOOST_REQUIRE(p_solver->get_solution().empty());

            p_solver->maximize();
            const auto solution = p_solver->get_solution();

            BOOST_REQUIRE_CLOSE(solution[0], expected_solution[0], c_eps);
            BOOST_REQUIRE_CLOSE(solution[1], expected_solution[1], c_eps);
        }
        const auto end_time = GetTickCount();

        if (LOGGING)
            cout << "Test for multiple solves took " << end_time - start_time << " ms" << endl;
    }


    void test_performance_big(ILPSolverInterface* p_solver)
    {
        static constexpr int c_num_constraints{ 50 };
        static constexpr int c_num_variables  { 50000 };
        const auto start_time = GetTickCount();

        auto [var_time, cons_time] = generate_random_problem(p_solver, c_num_variables, c_num_constraints);

        // Check before finalizing.
        BOOST_REQUIRE_EQUAL( p_solver->get_num_constraints(), c_num_constraints );
        BOOST_REQUIRE_EQUAL( p_solver->get_num_variables(),   c_num_variables );

        const auto middle_time = GetTickCount();
        p_solver->set_max_seconds(0.001);
        p_solver->minimize();

        // Check after finalizing.
        BOOST_REQUIRE_EQUAL(p_solver->get_num_constraints(), c_num_constraints);
        BOOST_REQUIRE_EQUAL(p_solver->get_num_variables(),   c_num_variables);

        const auto end_time = GetTickCount();

        if (LOGGING)
            cout << "Test for creating a big problem took " << end_time - start_time << " ms.\n"
                 << "\t" <<  var_time              << " for creating the variables.\n"
                 << "\t" << cons_time              << " for creating the constraints.\n"
                 << "\t" << end_time - middle_time << " for finalizing the problem." << endl;

    }


    void test_performance_zero(ILPSolverInterface* p_solver)
    {
        const auto start_time = GetTickCount();
        for(int i = 0; i < 1000; i++)
            p_solver->add_variable_integer(1., 0., 2.);
        p_solver->add_variable_integer(-1., 0., 2.);

        std::vector<double> constraint(1001, 0.);
        constraint[0] = 1.;
        p_solver->add_constraint(constraint, -1., 1.);
        for (int j = 1; j < 1001; j++)
        {
            constraint[j-1] = 0.;
            constraint[j]   = 1.;
            p_solver->add_constraint(constraint, -1., 1.);
        }
        p_solver->minimize();
        const auto objective = p_solver->get_objective();

        const auto end_time = GetTickCount();
        BOOST_REQUIRE_CLOSE( objective, -1., c_eps );

        if (LOGGING)
            cout << "Test for zero-pruning took " << end_time - start_time << " ms" << endl;
    }


    void test_start_solution(ILPSolverInterface* p_solver, double p_sense)
    {
        // max x+y+2z (<=> min -(x+y+2z)), 0 <= x, y, z <= 2
        p_solver->add_variable_integer(1*p_sense, 0, 2);
        p_solver->add_variable_integer(1*p_sense, 0, 2);
        p_solver->add_variable_integer(2*p_sense, 0, 2);

        // x+z <= 2
        vector<double> values_1{1., 0., 1.};
        p_solver->add_constraint_upper(values_1, 2);

        // y+z <= 2
        vector<double> values_2{0., 1., 1.};
        p_solver->add_constraint_upper(values_2, 2);

        // x+y+2z = (x+z) + (y+z), i.e., optimum <= 4. Optimum is attained at (0,0,2), (1,1,1), (2,2,0)
        vector<double> expected_solution{0., 0., 2.};

        p_solver->set_presolve(false);

        for (auto i = 0; i < 3; ++i)
        {
            p_solver->reset_solution();
            p_solver->set_start_solution(expected_solution);

            if (p_sense > 0)
                p_solver->maximize();
            else
                p_solver->minimize();
            const auto solution = p_solver->get_solution();

            BOOST_REQUIRE_CLOSE (solution[0], expected_solution[0], c_eps);
            BOOST_REQUIRE_CLOSE (solution[1], expected_solution[1], c_eps);
            BOOST_REQUIRE_CLOSE (solution[2], expected_solution[2], c_eps);

            // Iterate: (0,0,2) -> (1,1,1) -> (2,2,0)
            expected_solution[0] += 1.0;
            expected_solution[1] += 1.0;
            expected_solution[2] -= 1.0;
        }

    }


    void test_start_solution_minimization(ILPSolverInterface* p_solver)
    {
        test_start_solution(p_solver, -1.0);
    }


    void test_start_solution_maximization(ILPSolverInterface* p_solver)
    {
        test_start_solution(p_solver, 1.0);
    }


    void test_abs_gap_limit(ILPSolverInterface* p_solver)
    {
        double      gap{ 1.5 };
        double      ub{ 2.};
        std::vector obj{ 1., 0.5 };
        std::vector row{ 1., 0.5 };
        std::vector expected_sol{ 1., 1. }; // Expected when given as start solution due to gap.

        p_solver->set_presolve(false);
        p_solver->set_max_rel_gap(0.);

        p_solver->add_variable_integer(obj[0], 0., 1.);
        p_solver->add_variable_integer(obj[1], 0., 2.);

        p_solver->add_constraint_upper(row, ub);

        p_solver->set_start_solution(expected_sol);
        p_solver->maximize();
        auto sol = p_solver->get_solution();

        BOOST_REQUIRE_CLOSE (sol[0], 1., c_eps);
        BOOST_REQUIRE_CLOSE (sol[1], 2., c_eps);

        double best_objective{ p_solver->get_objective() };

        if (LOGGING)
            cout << "Maximal Absolute gap: " << gap << '\n'
                 << "  Start solution gap: " << fabs(best_objective - (expected_sol[0] * obj[0] + expected_sol[1] * obj[1])) << '\n'
                 << "   Best Solution gap: " << fabs(best_objective - (sol[0] * obj[0] + sol[1] * obj[1])) << std::endl;

        p_solver->reset_solution();

        p_solver->set_max_abs_gap(gap);
        p_solver->set_start_solution(expected_sol);

        p_solver->maximize();
        sol = p_solver->get_solution();
        BOOST_REQUIRE_CLOSE (sol[0], expected_sol[0], c_eps);
        BOOST_REQUIRE_CLOSE (sol[1], expected_sol[1], c_eps);

        if (LOGGING)
            cout << "  Found solution gap: " << fabs(best_objective - (sol[0] * obj[0] + sol[1] * obj[1])) << std::endl;
    }


    void test_rel_gap_limit(ILPSolverInterface* p_solver)
    {
        double      gap{ 0.1 };
        double      ub{ 2. };
        std::vector obj{ 10., 1. };
        std::vector row{ 1., 0.5 };
        std::vector expected_sol{ 1., 1. }; // Expected when given as start solution due to gap.

        p_solver->set_presolve(false);
        p_solver->set_max_abs_gap(0.);

        p_solver->add_variable_integer(obj[0], 0., 1.);
        p_solver->add_variable_integer(obj[1], 0., 2.);

        p_solver->add_constraint_upper(row, ub);

        p_solver->set_start_solution(expected_sol);
        p_solver->maximize();
        auto sol = p_solver->get_solution();

        BOOST_REQUIRE_CLOSE (sol[0], 1., c_eps);
        BOOST_REQUIRE_CLOSE (sol[1], 2., c_eps);

        double best_objective{ p_solver->get_objective() };

        if (LOGGING)
            cout << "Maximal Relative gap: " << gap << '\n'
                 << "  Start solution gap: " << 1. - (expected_sol[0] * obj[0] + expected_sol[1] * obj[1]) / best_objective << '\n'
                 << "   Best Solution gap: " << 1. - (sol[0] * obj[0] + sol[1] * obj[1]) / best_objective << '\n';

        p_solver->reset_solution();

        p_solver->set_max_rel_gap(gap);
        p_solver->set_start_solution(expected_sol);

        p_solver->maximize();
        sol = p_solver->get_solution();
        BOOST_REQUIRE_CLOSE (sol[0], expected_sol[0], c_eps);
        BOOST_REQUIRE_CLOSE (sol[1], expected_sol[1], c_eps);

        if (LOGGING)
            cout << "  Found solution gap: " << 1. - (sol[0] * obj[0] + sol[1] * obj[1])  / best_objective << '\n';
    }


    void test_bad_alloc(ILPSolverInterface* p_solver)
    {
        srand(3);
        p_solver->set_num_threads(8);
        // It is not clear that this is sufficient to provoke a bad_alloc.
        generate_random_problem(p_solver, 500000, 150);

        p_solver->set_max_seconds(10); // Don't waste time if we can build the problem.
        try
        {
            // bad_alloc should be treated as "no solution"
            p_solver->minimize();

            BOOST_REQUIRE(p_solver->get_status() == SolutionStatus::NO_SOLUTION);
            BOOST_REQUIRE_EQUAL (p_solver->get_solution().size(), 0u);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            BOOST_FAIL("Bad alloc test failed (threw exception instead of treating as >>no solution<<.");
        }
        catch (...)
        {
            std::cerr << "Stub threw unknown exception." << std::endl;
            BOOST_FAIL("Bad alloc test failed (threw exception instead of treating as >>no solution<<.");
        }
    }


    ILPSolverInterface* __stdcall create_stub()
    {
        constexpr std::string_view solver_exe_name = "ScaiIlpExe.exe";
        return create_solver_stub(solver_exe_name.data());
    }
}


namespace
{
    using namespace ilp_solver;

    // We need this because capturing lambdas are not usable for function pointers
    // and we somehow need to get a different file path per solver.
    int global_current_index{0};

    constexpr int num_solvers = 2 * (WITH_CBC) + (WITH_SCIP)
#if _WIN64 == 1
                              + (WITH_GUROBI)
#endif
    ;

    constexpr std::array<std::pair<FactoryFunction, std::string_view>, num_solvers> all_solvers
    {
#if WITH_CBC == 1
        std::pair{create_solver_cbc,    "CBC"},
        std::pair{create_stub,          "CBCStub"},
#endif
#if WITH_SCIP == 1
        std::pair{create_solver_scip,   "SCIP"},
#endif
#if (WITH_GUROBI == 1) && (_WIN64 == 1)
        std::pair{create_solver_gurobi, "Gurobi"},
#endif
    };
}


int create_ilp_test_suite()
{
    constexpr std::array<std::pair<TestFunction, std::string_view>, 9> all_tests
    { std::pair{test_sorting,                     "Sorting"}
    , std::pair{test_linear_programming,          "LinProgr"}
    , std::pair{test_start_solution_minimization, "StartSolutionMin"}
    , std::pair{test_start_solution_maximization, "StartSolutionMax"}
    , std::pair{test_abs_gap_limit,               "AbsGapLimit"}
    , std::pair{test_rel_gap_limit,               "RelGapLimit"}
    , std::pair{test_performance,                 "Performance"}
    , std::pair{test_performance_big,             "PerformanceBig"}
    , std::pair{test_performance_zero,            "PerformanceZero"}
    };

    boost::unit_test::test_suite* IlpSolverT = BOOST_TEST_SUITE("IlpSolverT");

    // Create a test suite for each kind of solver.
    for (auto& [solver, solver_name] : all_solvers )
    {
        boost::unit_test::test_suite* suite = BOOST_TEST_SUITE(solver_name.data());
        for (auto& [test, test_name] : all_tests)
        {
            // We need an object to pass to make_test_case, and it needs copy-by-value (solver and test are merely pointers anyway.).
            auto lambda = [solver, test]() { execute_test_and_destroy_solver(solver(), test); };
            // Since we use a functor and a name, we avoid using one of the macros and create a test case directly.
            suite->add( boost::unit_test::make_test_case(lambda, (std::string(solver_name) + '_' + test_name.data()).c_str(), __FILE__, __LINE__) );
        }

        auto mps_path   = [](ILPSolverInterface* p_solver) -> void {test_mps_output(p_solver, std::string(all_solvers[global_current_index++].second) + "_unittest.mps");};
        auto mps_lambda = [solver, mps_path]() { execute_test_and_destroy_solver(solver(), mps_path); };
        suite->add( boost::unit_test::make_test_case(mps_lambda, (std::string(solver_name) + "_MPSOut").c_str(), __FILE__, __LINE__) );

        if (solver_name.rfind("Stub") != std::string::npos)
        {
            auto lambda = [solver]() { execute_test_and_destroy_solver(solver(), test_bad_alloc); };
            suite->add( boost::unit_test::make_test_case(lambda, (std::string(solver_name) + "_BadAlloc").c_str(), __FILE__, __LINE__) );
        }

        // Add the current solver to the IlpSolverT test suite.
        IlpSolverT->add( suite );
    }

    // Add the whole IlpSolver test suite to the master test suite.
    boost::unit_test::framework::master_test_suite().add(IlpSolverT);

    return 0;
}


// Automatic registration of the test suites.
namespace
{
    inline const int hidden_registrar{ create_ilp_test_suite() };
}
