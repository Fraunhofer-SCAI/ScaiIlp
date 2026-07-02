#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include "utility.hpp"

#include <algorithm>
#include <array>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>


const auto c_eps = 0.0001;
const auto c_num_performance_test_repetitions = 1;

const bool LOGGING = true;

namespace ilp_solver
{
    static int round(double x)
    {
        return static_cast<int>(x + 0.5);
    }


    static double rand_double()
    {
        return static_cast<double>(rand())/RAND_MAX - 0.5;
    }


    static std::pair<long long, long long> generate_random_problem(ILPSolverInterface* p_solver, int p_num_variables, int p_num_constraints)
    {
        srand(3);
        static constexpr double variable_scaling = 10.0;
        static const double   constraint_scaling = p_num_variables * variable_scaling;

        const auto start_time = std::chrono::steady_clock::now();

        for (auto j = 0; j < p_num_variables; ++j)
            p_solver->add_variable_integer(rand_double(), variable_scaling*rand_double(), variable_scaling*(1.0 + rand_double()));

        const auto middle_time = std::chrono::steady_clock::now();

        std::vector<double> constraint_vector(p_num_variables);

        for (auto i = 0; i < p_num_constraints; ++i)
        {
            std::ranges::generate(constraint_vector, rand_double);
            p_solver->add_constraint(constraint_vector, constraint_scaling*rand_double(), constraint_scaling*(1.0 + rand_double()));
        }
        const auto end_time = std::chrono::steady_clock::now();

        const auto time_variables   = std::chrono::duration_cast<std::chrono::milliseconds>(middle_time -  start_time).count();
        const auto time_constraints = std::chrono::duration_cast<std::chrono::milliseconds>(   end_time - middle_time).count();
        return {time_variables, time_constraints};
    }


    void test_sorting(ILPSolverInterface* p_solver)
    {
        std::stringstream logging;

        const std::vector<int> numbers{62, 20, 4, 49, 97, 73, 35, 51, 18, 86};
        const auto num_vars = isize(numbers);

        // xi - target position of numbers[i]
        //
        // min x0 + ... + x9
        // s.t. xk - xl >= 0.1 for every (k,l) for which numbers[k] > numbers[l]
        // (Note: If we set the rhs to 1.0, then the integrality constraint is superfluous.)
        //      xi >= 0 integral

        // Add variables
        for (auto i = 0; i < num_vars; ++i)
            p_solver->add_variable_integer(1, 0, std::numeric_limits<int>::max(), "x" + std::to_string(i));

        // Add constraints
        logging << "Initial array: ";

        const std::vector<double> values{1., -1.};
        std::vector<int> indices{0, 0};
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
        logging << "\n" << std::endl;

        p_solver->minimize();

        const auto obj_value   = p_solver->get_objective();
        const auto permutation = p_solver->get_solution();
        const auto status      = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        logging << "Solution is " << (optimal ? "" : "not ") << "optimal" << std::endl;
        BOOST_REQUIRE(optimal);

        // Check correctness of objective
        const auto expected_obj_value = num_vars*(num_vars - 1)/2;
        BOOST_REQUIRE_CLOSE(obj_value, expected_obj_value, c_eps);  // objective should be 0+1+...+(num_numbers-1)

        // Check correctness of solution
        auto sorted = std::vector<int>(num_vars, INT_MIN);          // sort according solution

        logging << std::endl << "Resulting permutation: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            const auto pos = round(permutation[i]);

            logging << pos << " ";

            BOOST_REQUIRE_CLOSE(pos, permutation[i], c_eps);        // solution must be integral
            sorted[pos] = numbers[i];
        }
        logging << std::endl;

        logging << "Sorted array: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            logging << sorted[i] << " ";

            BOOST_REQUIRE_NE(sorted[i], INT_MIN);                   // solution must be a permutation
            if (i > 0)
                BOOST_REQUIRE_LT(sorted[i-1], sorted[i]);           // solution must sort
        }
        logging << std::endl;

        if (LOGGING)
            std::cout << logging.str();
    }


    void test_linear_programming(ILPSolverInterface* p_solver)
    {
        std::stringstream logging;

        const auto num_vars = 5;
        const auto num_dirs = num_vars;
        const auto num_cons = 7;
        const auto constraint_shift = 10.0;

        // expected solution
        double x0[num_vars] = {2.72, 42.0, -1.41, 3.14, -1.62};

        // expected dual solution
        double y0[num_cons] = {7, 0, 2, 5, 0, 6, 3};

        // constraint directions
        double a[num_dirs][num_vars] = {{1.24, -3.47, 8.32, 4.78, -5.34},
                                        {-7.23, 4.90, -3.21, 0.39, 9.45},
                                        {2.40, 9.38, -6.67, -6.43, 5.38},
                                        {-4.79, 1.47, 6.47, 4.30, -8.39},
                                        {8.32, -7.20, 4.96, -9.41, 3.64}};

        double scalar[num_dirs] = {7, 2, 5, 6, 3};

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
        logging << std::endl;

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
        logging << "Constraints:" << std::endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            const auto values = std::vector<double>(a[j], a[j] + num_vars);

            auto j_s{std::to_string(j)};
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
            logging << " <= " << b[j] << std::endl;
        }
        logging << std::endl;

        p_solver->maximize();

        const auto obj    = p_solver->get_objective();
        const auto x      = p_solver->get_solution();
        const auto y      = p_solver->get_dual_sol();
        const auto status = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        BOOST_REQUIRE(optimal);
        logging << "The solution is " << (optimal ? "" : "not ") << "optimal." << std::endl;

        // Check correctness of objective
        auto obj_cmp = 0.0;
        for (auto i = 0; i < num_vars; ++i)
            obj_cmp += c[i]*x[i];
        BOOST_REQUIRE_CLOSE(obj, obj_cmp, c_eps);   // objective should fit to the solution

        logging << "\nExpected objective: " << obj0 << "\n"
                <<  "Resulting objective: " << obj  << std::endl;

        BOOST_REQUIRE_CLOSE(obj, obj0, c_eps);      // objective should equal the optimal objective

        // Check correctness of solution
        logging << "\nConstraint values:" << std::endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            auto constraint_value = 0.0;
            for (auto i = 0; i < num_vars; ++i)
                constraint_value += a[j][i]*x[i];

            logging << constraint_value << " (must be in [" << b[j] - constraint_shift << "," << b[j] << "), expected "
                    << b[j] << std::endl;

            BOOST_REQUIRE_GE(constraint_value, b[j] - constraint_shift - c_eps);  // solution obeys lower bound of the j'th constraint
            BOOST_REQUIRE_LE(constraint_value, b[j] + c_eps);                     // solution obeys upper bound of the j'th constraint
            BOOST_REQUIRE_GE(constraint_value, b[j] - c_eps);                     // upper bound of the j'th constraint is tight
        }

        logging << "\nExpected solution: ";
        for (auto i = 0; i < num_vars; ++i)
            logging << x0[i] << " ";
        logging << std::endl;

        logging << "Resulting solution: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            logging << x[i] << " ";
            BOOST_REQUIRE_CLOSE(x[i], x0[i], c_eps);
        }
        logging << std::endl;

        logging << "\nExpected dual solution: ";
        for (auto j = 0; j < num_cons; ++j)
            logging << y0[j] << " ";
        logging << std::endl;

        logging << "Resulting dual solution: ";
        for (auto j = 0; j < isize(y); ++j)
        {
            logging << y[j] << " ";
            BOOST_REQUIRE_CLOSE(y[j], y0[j], c_eps);
        }
        logging << std::endl;

        if (LOGGING)
            std::cout << logging.str();
    }


    // Tests only for file creation and non-null-size.
    // Correctness of file can only be tested if we implement reading from mps, too.
    void test_mps_output(ILPSolverInterface* p_solver, const std::string& p_path)
    {
        // Disable output for problem construction and solve.
        std::cout.setstate(std::ios_base::failbit);
        test_linear_programming(p_solver);
        std::cout.clear();
        boost::filesystem::path path{p_path};
        boost::filesystem::remove(path);
        p_solver->print_mps_file(p_path);
        BOOST_REQUIRE(boost::filesystem::is_regular_file(path));
        auto size{ boost::filesystem::file_size(path) };
        BOOST_REQUIRE(size > 0);
        if (LOGGING)
        {
            std::cout << "Successfully wrote mps-File to " << boost::filesystem::absolute(path).generic_string() << ".\n"
                      << "\tFilesize is " << size << " Byte." << std::endl;
        }
    }


    void test_performance(ILPSolverInterface* p_solver)
    {
        // max x+y, -1 <= x, y <= 1
        p_solver->add_variable_continuous(1, -1, 1);
        p_solver->add_variable_continuous(1, -1, 1);

        // x+2y <= 2
        const std::vector<double> values_1{1, 2};
        p_solver->add_constraint_upper(values_1, 2);

        // 2x+y <= 2
        const std::vector<double> values_2{2,1};
        p_solver->add_constraint_upper(values_2, 2);

        const std::vector<double> expected_solution{2./3., 2./3.};

        const auto start_time = std::chrono::steady_clock::now();
        for (auto i = 1; i <= c_num_performance_test_repetitions; ++i)
        {
            p_solver->reset_solution();
            BOOST_REQUIRE(p_solver->get_status() != SolutionStatus::PROVEN_OPTIMAL);
            BOOST_REQUIRE(p_solver->get_solution().empty());

            p_solver->maximize();
            const auto solution = p_solver->get_solution();

            BOOST_REQUIRE_CLOSE(solution[0], expected_solution[0], c_eps);
            BOOST_REQUIRE_CLOSE(solution[1], expected_solution[1], c_eps);
        }
        const auto end_time = std::chrono::steady_clock::now();

        if (LOGGING)
        {
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            std::cout << "Test for multiple solves took " << time << " ms" << std::endl;
        }
    }


    void test_performance_big(ILPSolverInterface* p_solver)
    {
        static constexpr int c_num_constraints{50};
        static constexpr int c_num_variables{50000};
        const auto           start_time = std::chrono::steady_clock::now();

        auto [var_time, cons_time] = generate_random_problem(p_solver, c_num_variables, c_num_constraints);

        // Check before finalizing.
        BOOST_REQUIRE_EQUAL(p_solver->get_num_constraints(), c_num_constraints);
        BOOST_REQUIRE_EQUAL(p_solver->get_num_variables(),   c_num_variables);

        const auto middle_time = std::chrono::steady_clock::now();
        p_solver->set_max_seconds(0.001);
        p_solver->minimize();

        // Check after finalizing.
        BOOST_REQUIRE_EQUAL(p_solver->get_num_constraints(), c_num_constraints);
        BOOST_REQUIRE_EQUAL(p_solver->get_num_variables(),   c_num_variables);

        const auto end_time = std::chrono::steady_clock::now();

        if (LOGGING)
        {
            const auto time_creating = std::chrono::duration_cast<std::chrono::milliseconds>(middle_time - start_time).count();
            const auto time_solving  = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - middle_time).count();
            std::cout << "Test for creating a big problem took " << time_creating << " ms.\n"
                      << "\t" << var_time     << " for creating the variables.\n"
                      << "\t" << cons_time    << " for creating the constraints.\n"
                      << "\t" << time_solving << " for finalizing the problem."   << std::endl;
        }
    }


    void test_performance_zero(ILPSolverInterface* p_solver)
    {
        const auto start_time = std::chrono::steady_clock::now();
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

        const auto end_time = std::chrono::steady_clock::now();
        BOOST_REQUIRE_CLOSE(objective, -1., c_eps);

        if (LOGGING)
        {
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            std::cout << "Test for zero-pruning took " <<  time << " ms" << std::endl;
        }
    }


    void test_start_solution(ILPSolverInterface* p_solver, double p_sense)
    {
        // max x+y+2z (<=> min -(x+y+2z)), 0 <= x, y, z <= 2
        p_solver->add_variable_integer(1*p_sense, 0, 2);
        p_solver->add_variable_integer(1*p_sense, 0, 2);
        p_solver->add_variable_integer(2*p_sense, 0, 2);

        // x+z <= 2
        const std::vector<double> values_1{1., 0., 1.};
        p_solver->add_constraint_upper(values_1, 2);

        // y+z <= 2
        const std::vector<double> values_2{0., 1., 1.};
        p_solver->add_constraint_upper(values_2, 2);

        // Check that an invalid solution raises an exception.
        // This may not be implemented for all solvers, but it is for CBC and HiGHS.
        // Normally the exception will be raised in set_start_solution.
        // If the stub is used, it will be thrown after the solution is given to the solver in the external process
        // which is invoked from maximize().
        const std::vector<double> invalid_solution{1., 1., 2.};
        BOOST_CHECK_THROW(p_solver->set_start_solution(invalid_solution); p_solver->maximize(), InvalidStartSolutionException);

        // x+y+2z = (x+z) + (y+z), i.e., optimum <= 4. Optimum is attained at (0,0,2), (1,1,1), (2,2,0)
        std::vector<double> valid_solution{0., 0., 2.};

        for (auto i = 0; i < 3; ++i)
        {
            p_solver->reset_solution();
            // set_start_solution (or maximize - see comment above) should throw on failure.
            BOOST_CHECK_NO_THROW(p_solver->set_start_solution(valid_solution); p_solver->maximize());

            // Iterate: (0,0,2) -> (1,1,1) -> (2,2,0)
            valid_solution[0] += 1.0;
            valid_solution[1] += 1.0;
            valid_solution[2] -= 1.0;
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


    void test_cutoff(ILPSolverInterface* p_solver)
    {
        const std::vector obj{1., 1.};
        const std::vector row{1., 1.};

        p_solver->set_presolve(false);

        p_solver->add_variable_integer(obj[0], 0., 10.);
        p_solver->add_variable_integer(obj[1], 0., 10.);

        p_solver->add_constraint_lower(row, 1.5);
        p_solver->set_cutoff(1.9);

        p_solver->minimize();
        auto sol    = p_solver->get_solution();
        auto status = p_solver->get_status();

        BOOST_REQUIRE(status == SolutionStatus::PROVEN_INFEASIBLE);
    }


    void test_dual_bound(ILPSolverInterface* p_solver)
    {
        // Trivial integer program: Solved optimally - thus the dual bound is equal to the objective value.
        const std::vector obj{1., 1.};
        const std::vector row{1., 2.};

        p_solver->set_presolve(false);
        p_solver->add_variable_integer(obj[0], 0., 4.);
        p_solver->add_variable_integer(obj[1], 0., 5.);
        p_solver->add_constraint_lower(row, 3.);
        p_solver->minimize();

        const auto objective  = p_solver->get_objective();
        const auto dual_bound = p_solver->get_dual_bound();

        // Check if the dual bound is equal to the objective. This holds because the solution is optimal.
        BOOST_REQUIRE_CLOSE(objective, dual_bound, c_eps);
    }


    void test_dual_bound_limited_nodes(ILPSolverInterface* p_solver)
    {
        // More difficult integer program (AI generated): Due to the small node limit, may not be solved optimally.
        // Thus the dual bound may be smaller than the objective value.

        const std::vector obj{63., 44., 84., 49., 62., 77., 57., 92., 48., 67., 81., 39., 73., 55., 96., 42., 88.,
                              71., 60., 85., 53., 78., 46., 91., 64., 37., 82., 59., 74., 68., 94., 41., 87., 56.,
                              79., 50., 93., 45., 66., 83., 52., 76., 61., 38., 89., 54., 69., 95., 47., 72.};
        const std::vector row{31., 22., 42., 25., 30., 38., 28., 46., 24., 33., 40., 19., 36., 27., 48., 21., 44.,
                              35., 29., 43., 26., 39., 23., 45., 32., 18., 41., 29., 37., 34., 47., 20., 43., 28.,
                              39., 25., 46., 22., 33., 41., 26., 38., 30., 19., 44., 27., 34., 47., 23., 36.};

        p_solver->set_presolve(false);
        p_solver->set_max_nodes(1);
        p_solver->set_max_seconds(10);

        for (int i = 0; i < 50; i++)
            p_solver->add_variable_integer(obj[i], 0., 1.);

        p_solver->add_constraint_lower(row, 11.);
        p_solver->minimize();

        const auto objective        = p_solver->get_objective();
        const auto dual_bound       = p_solver->get_dual_bound();
        const auto optimal_solution = 37.;
        // Check if the dual bound is larger (or equal to) the optimal solution and larger than 0.
        BOOST_REQUIRE_GE(objective + c_eps, optimal_solution);
        BOOST_REQUIRE_GE(optimal_solution + c_eps, dual_bound);
        BOOST_REQUIRE_GT(dual_bound, 0.);
    }


    void test_bad_alloc(ILPSolverInterface* p_solver)
    {
        srand(3);
        p_solver->set_num_threads(8);
        p_solver->set_max_seconds(10); // Don't waste time if we can build the problem.

        // It is not clear that this is sufficient to provoke a bad_alloc.
        try
        {
            generate_random_problem(p_solver, 500000, 150);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            BOOST_FAIL("Bad alloc test failed (Could not create problem of the required size).");
        }
        catch (...)
        {
            std::cerr << "Stub threw unknown exception." << std::endl;
            BOOST_FAIL("Bad alloc test failed (Could not create problem of the required size).");
        }

        try
        {
            // bad_alloc should be treated as "no solution"
            p_solver->minimize();

            BOOST_REQUIRE(p_solver->get_status() == SolutionStatus::NO_SOLUTION);
            BOOST_REQUIRE_EQUAL(p_solver->get_solution().size(), 0u);
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


    void test_set_bounds(ILPSolverInterface* p_solver)
    {
        // Positive objective value -> test upper bound
        p_solver->add_variable_integer( 16, 0, c_pos_inf);
        p_solver->add_variable_boolean( 4);
        p_solver->add_variable_continuous( 1, 0, c_pos_inf);
        // Negative objective value -> test lower bound
        p_solver->add_variable_integer(-16, c_neg_inf, 0);
        p_solver->add_variable_boolean(-4);
        p_solver->add_variable_continuous(-1, c_neg_inf, 0);

        // Bound all variables such that the objective preference is int > bool > cont
        p_solver->add_constraint_upper(std::array<double,6> {9., 3., 1., 0., 0., 0.},  23.);
        p_solver->add_constraint_lower(std::array<double,6> {0., 0., 0., 9., 3., 1.}, -20.);

        // First find solution with unbounded variables
        p_solver->maximize();
        std::vector<double> solution = p_solver->get_solution();
        for(int i = 0; i < 6; i++)
            BOOST_REQUIRE_CLOSE(solution[i], (std::array<double,6> {2., 1., 2., -2., 0., -2.})[i], c_eps);
        p_solver->reset_solution();

        // Bound variables, which makes the constraints slack
        p_solver->set_bound_upper(0,  1.);
        p_solver->set_bound_upper(2,  1.);
        p_solver->set_bound_lower(3, -1.);
        p_solver->set_bound_lower(5, -1.);

        p_solver->maximize();

        // Check that the new solutions respects the variable bounds
        solution = p_solver->get_solution();
        for (int i = 0; i < 6; i++)
            BOOST_REQUIRE_CLOSE(solution[i], (std::array<double, 6>{1., 1., 1., -1., 0., -1.})[i], c_eps);
    }


    void test_column_generation(ILPSolverInterface* p_main_solver, ILPSolverInterface* p_knapsack_solver)
    {
        // Parameters for a bin packing problem, taken from https://scipbook.readthedocs.io/en/latest/bpp.html
        constexpr double bin_size = 9;
        constexpr std::array<double,7> items  = {2., 3., 4., 5., 6., 7., 8.};
        constexpr std::array<double,7> counts = {4., 2., 6., 6., 2., 2., 2.};

        std::vector<std::vector<double>> confs;         // Set of configurations

        for (auto count : counts)                       // Setup main problem
            p_main_solver->add_constraint_equality({}, count);
        for (size_t i = 0; i < items.size(); ++i)
        {
            std::vector<double> conf(items.size(), 0.);
            conf[i] = 1.; // For denser start solution: std::min(count[i], bin_size / items[i]);
            confs.push_back(conf);
            p_main_solver->add_variable_continuous(conf, 1., 0., ilp_solver::c_pos_inf_bound);
        }

        for (const double ub : counts)                  // Setup knapsack problem
            p_knapsack_solver->add_variable_integer(1., 0., ub);
        p_knapsack_solver->add_constraint_upper(items, bin_size);

        // Solve repeatedly, adding variables, until no improving variable is left.
        int iters = 0;
        std::chrono::milliseconds knapsack_time{0};
        while (true)
        {
            p_main_solver->minimize();
            if (LOGGING)
                std::cout << "Iteration " << iters++ << " found relaxation with value: " << p_main_solver->get_objective() << "\n";
            std::vector<double> dual_sol = p_main_solver->get_dual_sol();
            if (!dual_sol.size())
            {
                std::cerr << "No valid dual solution obtained from solver!\n";
                return;
            }
            auto start_time = std::chrono::high_resolution_clock::now();
            p_knapsack_solver->reset_solution();
            p_knapsack_solver->set_objective(dual_sol);
            p_knapsack_solver->maximize();
            auto end_time = std::chrono::high_resolution_clock::now();
            knapsack_time += std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            if (p_knapsack_solver->get_objective() < 1.001)
              break;
            std::vector<double> sol = p_knapsack_solver->get_solution();
            confs.push_back(sol);
            p_main_solver->reset_solution();
            p_main_solver->add_variable_continuous(sol, 1, 0, ilp_solver::c_pos_inf_bound);
        }
        if (LOGGING)
            std::cout << "Done. Knapsack solving took " << knapsack_time.count() << "ms\n";
        BOOST_CHECK_CLOSE(p_main_solver->get_objective(), 12.5, c_eps);

        // Set main problem variables to integer and solve again
        int varcount = p_main_solver->get_num_variables();
        for (int i = 0; i < varcount; i++)
            p_main_solver->set_variable_type(i, VariableType::INTEGER);
        p_main_solver->reset_solution();
        p_main_solver->minimize();
        if (LOGGING)
            std::cout << "Solve integer found solution with value: " << p_main_solver->get_objective() << "\n";
        BOOST_CHECK_CLOSE(p_main_solver->get_objective(), 13, c_eps);
    }
}


int create_ilp_test_suite()
{
    using namespace ilp_solver;
    constexpr std::array<std::pair<void (*)(ILPSolverInterface*), std::string_view>, 11> all_tests
    { std::pair{test_sorting,                     "Sorting"}
    , std::pair{test_linear_programming,          "LinProgr"}
    , std::pair{test_start_solution_minimization, "StartSolutionMin"}
    , std::pair{test_start_solution_maximization, "StartSolutionMax"}
    , std::pair{test_cutoff,                      "CutOff"}
    , std::pair{test_performance,                 "Performance"}
    , std::pair{test_performance_big,             "PerformanceBig"}
    , std::pair{test_performance_zero,            "PerformanceZero"}
    , std::pair{test_set_bounds,                  "SetBounds"}
    , std::pair{test_dual_bound,                  "DualBound"}
    , std::pair{test_dual_bound_limited_nodes,    "DualBoundLimitedNodes"}
    };

    boost::unit_test::test_suite* IlpSolverT = BOOST_TEST_SUITE("IlpSolverT");

    // Create a test suite for each kind of solver.
    for (auto& [solver, solver_name] : all_solvers)
    {
        boost::unit_test::test_suite* suite = BOOST_TEST_SUITE(solver_name.data());
        for (auto& [test, test_name] : all_tests)
        {
            // We need an object to pass to make_test_case, and it needs copy-by-value (solver and test are merely pointers anyway.).
            auto lambda = [solver, test]() { test(solver().get()); };
            // Since we use a functor and a name, we avoid using one of the macros and create a test case directly.
            suite->add(boost::unit_test::make_test_case(lambda, (std::string(solver_name) + '_' + test_name.data()).c_str(), __FILE__, __LINE__));
        }

        auto mps_lambda = [solver, solver_name]() { test_mps_output(solver().get(), std::string(solver_name) + "_unittest.mps"); };
        suite->add(boost::unit_test::make_test_case(mps_lambda, (std::string(solver_name) + "_MPSOut").c_str(), __FILE__, __LINE__));

        auto cg_lambda = [solver]() { test_column_generation(solver().get(), solver().get()); };
        suite->add(boost::unit_test::make_test_case(cg_lambda, (std::string(solver_name) + "_ColumnGeneration").c_str(), __FILE__, __LINE__));

        if (solver_name.rfind("Stub") != std::string::npos)
        {
            auto lambda = [solver]() { test_bad_alloc(solver().get()); };
            suite->add(boost::unit_test::make_test_case(lambda, (std::string(solver_name) + "_BadAlloc").c_str(), __FILE__, __LINE__));
        }

        // Add the current solver to the IlpSolverT test suite.
        IlpSolverT->add(suite);
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
