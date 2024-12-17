#include "ilp_solver_interface_t.hpp"

#include "ilp_solver_factory.hpp"
#include "ilp_solver_interface.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

#define NOMINMAX
#include <windows.h>    // for GetTickCount

using std::cout;
using std::endl;
using std::string;
using std::vector;

const auto c_eps = 0.0001;
const auto c_num_performance_test_repetitions = 10000;

namespace ilp_solver
{
    /*********************
    * Internal functions *
    *********************/
    static int round(double x)
    {
        return (int) (x+0.5);
    }


    static double rand_double()
    {
        return -0.5 + (1.0*rand())/RAND_MAX;
    }


    static string underline(string p_text)
    {
        std::fill(std::begin(p_text), std::end(p_text), '=');
        return p_text;
    }


    static void print_caption(const string& p_test_name, const string& p_solver_name)
    {
        cout << endl
             << p_test_name            << " (" << p_solver_name            << ")" << endl
             << underline(p_test_name) << "==" << underline(p_solver_name) << "=" << endl
             << endl;
    }


    /**********************
    * Published functions *
    **********************/
    void execute_test_and_destroy_solver(ILPSolverInterface* p_solver, const string& p_solver_name, std::function<void(ILPSolverInterface*, const string&)> p_test)
    {
        try
        {
            p_test(p_solver, p_solver_name);
        }
        catch (...)
        {
            destroy_solver(p_solver);
            throw;
        }
        destroy_solver(p_solver);
    }


    void test_sorting(ILPSolverInterface* p_solver, const string& p_solver_name)
    {
        print_caption("Sorting test", p_solver_name);

        int number_array[] = { 62, 20, 4, 49, 97, 73, 35, 51, 18, 86};
        const auto numbers = vector<int>(std::begin(number_array), std::end(number_array));
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
        cout << "Initial array: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            for (auto j = i+1; j < num_vars; ++j)
            {
                // Reorder i and j (naming them k and l) s.t. numbers[k] > numbers[l]
                auto k = i;
                auto l = j;
                if (numbers[k] < numbers[l])
                    std::swap(k, l);

                vector<int> indices;
                vector<double> values;

                indices.push_back(k);
                values.push_back(1);

                indices.push_back(l);
                values.push_back(-1);

                p_solver->add_constraint_lower(indices, values, 1.0, "x" + std::to_string(k) + ">x" + std::to_string(l));
            }
            cout << numbers[i] << " ";
        }
        cout << endl << endl;

        p_solver->minimize();

        const auto obj_value = p_solver->get_objective();
        const auto permutation = p_solver->get_solution();
        const auto status = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        cout << "The solution is " << (optimal ? "" : "not ") << "optimal." << endl;
        assert(optimal);

        // Check correctness of objective
        const auto expected_obj_value = num_vars*(num_vars-1)/2;
        assert(fabs(obj_value - expected_obj_value) < c_eps);  // objective should be 0+1+...+(num_numbers-1)

        // Check correctness of solution
        auto sorted = vector<int>(num_vars, INT_MIN);       // sort according solution

        cout << endl << "Resulting permutation: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            const auto pos = round(permutation[i]);

            cout << pos << " ";

            assert(fabs(pos - permutation[i]) < c_eps);    // solution must be integral
            sorted[pos] = numbers[i];
        }
        cout << endl;

        cout << "Sorted array: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            cout << sorted[i] << " ";

            assert(sorted[i] != INT_MIN);                   // solution must be a permutation
            if (i > 0)
                assert(sorted[i-1] < sorted[i]);            // solution must sort
        }
        cout << endl;
    }

    void test_linear_programming(ILPSolverInterface* p_solver, const string& p_solver_name)
    {
        print_caption("LP test", p_solver_name);

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

        cout << "Objective: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            c[i] = 0.0;
            for (auto j = 0; j < num_dirs; ++j)
                c[i] += scalar[j]*a[j][i];

            cout << c[i] << " ";
        }
        cout << endl;

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
        cout << "Constraints:" << endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            const auto values = vector<double>(a[j], a[j] + num_vars);

            p_solver->add_constraint_lower(values, b[j] - constraint_shift, "x*dir" + std::to_string(j) + " >= b" + std::to_string(j) + " - 10");
            p_solver->add_constraint_upper(values, b[j],                    "x*dir" + std::to_string(j) + " <= b" + std::to_string(j));

            cout << b[j] - constraint_shift << " <= ";
            for (auto i = 0; i < num_vars; ++i)
                cout << (i == 0 ? "" : " + ") << a[j][i] << "*x" << i;
            cout << " <= " << b[j] << endl;
        }
        cout << endl;

        p_solver->maximize();

        const auto obj = p_solver->get_objective();
        const auto x = p_solver->get_solution();
        const auto status = p_solver->get_status();

        // Check solution status
        const auto optimal = status == SolutionStatus::PROVEN_OPTIMAL;
        cout << "The solution is " << (optimal ? "" : "not ") << "optimal." << endl;
        assert(optimal);

        // Check correctness of objective
        auto obj_cmp = 0.0;
        for (auto i = 0; i < num_vars; ++i)
            obj_cmp += c[i]*x[i];
        assert(fabs(obj - obj_cmp) < c_eps);   // objective should fit to the solution

        cout << endl;
        cout << "Expected objective: " << obj0 << endl;
        cout << "Resulting objective: " << obj << endl;

        assert(fabs(obj - obj0) < c_eps);      // objective should equal the optimal objective

        // Check correctness of solution
        cout << endl << "Constraint values:" << endl;
        for (auto j = 0; j < num_dirs; ++j)
        {
            auto constraint_value = 0.0;
            for (auto i = 0; i < num_vars; ++i)
                constraint_value += a[j][i]*x[i];

            cout << constraint_value << " (must be in [" << b[j] -constraint_shift << "," << b[j] << "), expected " << b[j] << endl;

            assert(constraint_value >= b[j] - constraint_shift - c_eps);  // solution obeys lower bound of the j'th constraint
            assert(constraint_value <= b[j] + c_eps);                     // solution obeys upper bound of the j'th constraint
            assert(constraint_value >= b[j] - c_eps);                     // upper bound of the j'th constraint is tight
        }

        cout << endl << "Expected solution: ";
        for (auto i = 0; i < num_vars; ++i)
            cout << x0[i] << " ";
        cout << endl;

        cout << "Resulting solution: ";
        for (auto i = 0; i < num_vars; ++i)
        {
            cout << x[i] << " ";
            assert(fabs(x[i] - x0[i]) <= c_eps);
        }
        cout << endl;
    }


    void test_performance(ILPSolverInterface* p_solver, const string& p_solver_name)
    {
        print_caption("Performance test", p_solver_name);

        // max x+y, -1 <= x, y <= 1
        p_solver->add_variable_continuous(1, -1, 1);
        p_solver->add_variable_continuous(1, -1, 1);

        // x+2y <= 2
        vector<double> values_1;
        values_1.push_back(1);
        values_1.push_back(2);
        p_solver->add_constraint_upper(values_1, 2);

        // 2x+y <= 2
        vector<double> values_2;
        values_2.push_back(2);
        values_2.push_back(1);
        p_solver->add_constraint_upper(values_2, 2);

        vector<double> expected_solution;
        expected_solution.push_back(2.0/3.0);
        expected_solution.push_back(2.0/3.0);

        const auto one_twentieth = c_num_performance_test_repetitions/20;

        const auto start_time = GetTickCount();
        for (auto i = 1; i <= c_num_performance_test_repetitions; ++i)
        {
            p_solver->maximize();
            const auto solution = p_solver->get_solution();

            assert(fabs(solution[0] - expected_solution[0]) < c_eps);
            assert(fabs(solution[1] - expected_solution[1]) < c_eps);

            if ((i % one_twentieth) == 0)
                cout << "*";
        }
        const auto end_time = GetTickCount();

        cout << endl
             << endl
             << "Test took " << end_time - start_time << " ms" << endl;
    }


    void test_bad_alloc(ILPSolverInterface* p_solver, const string& p_solver_name)
    {
        print_caption("Bad alloc test", p_solver_name);

        srand(3);

        // It is not clear that this is sufficient to provoke a bad_alloc.
        const auto variable_scaling = 10.0;
        const auto num_variables = 500000;
        for (auto j = 0; j < num_variables; ++j)
            p_solver->add_variable_integer(rand_double(), variable_scaling*rand_double(), variable_scaling*(1.0 + rand_double()));

        const auto constraint_scaling = num_variables*variable_scaling;
        const auto num_constraints = 100;
        std::vector<double> constraint_vector(num_variables);
        for (auto i = 0; i < num_constraints; ++i)
        {
            std::generate(std::begin(constraint_vector), std::end(constraint_vector), [](){ return rand_double(); });
            p_solver->add_constraint(constraint_vector, constraint_scaling*rand_double(), constraint_scaling*(1.0 + rand_double()));
        }

        try
        {
            // bad_alloc should be treated as "no solution"
            p_solver->minimize();
            assert(p_solver->get_status() == SolutionStatus::NO_SOLUTION);
            assert(p_solver->get_solution().size() == 0);

            if ((p_solver->get_status() == SolutionStatus::NO_SOLUTION) && (p_solver->get_solution().size() == 0))
                cout << "Test succeeded." << endl;
            else
                cout << "Test failed." << endl;
        }
        catch (...)
        {
            assert(false);

            cout << "Test failed." << endl;
        }
    }
}
