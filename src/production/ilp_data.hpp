#ifndef _ILP_DATA_HPP
#define _ILP_DATA_HPP

#include "ilp_solver_interface.hpp"
#include "ilp_solver_interface_impl.hpp"

#include <limits>
#include <vector>

namespace ilp_solver
{
    struct ILPData
    {
        std::vector< std::vector<double> > matrix;  // Note: mx0 matrices can be stored, but 0xn matrices cannot.
        std::vector<double> objective;
        std::vector<double> variable_lower;
        std::vector<double> variable_upper;
        std::vector<double> constraint_lower;
        std::vector<double> constraint_upper;
        std::vector<VariableType> variable_type;
        ObjectiveSense objective_sense;

        std::vector<double> start_solution;

        int num_threads;
        bool deterministic;
        int log_level;
        double max_seconds;

        ILPData() : objective_sense(ObjectiveSense::MINIMIZE),
                    num_threads(0), deterministic(true), log_level(0), max_seconds(std::numeric_limits<double>::max()) {}
    };


    struct ILPSolutionData
    {
        std::vector<double> solution;
        double objective;
        SolutionStatus solution_status;

        ILPSolutionData()
            : objective(std::numeric_limits<double>::quiet_NaN()),
              solution_status(SolutionStatus::NO_SOLUTION)
            {}

        explicit ILPSolutionData(ObjectiveSense p_objective_sense)
            : objective(p_objective_sense == ObjectiveSense::MINIMIZE ? std::numeric_limits<double>::max()
                                                                      : std::numeric_limits<double>::lowest()),
              solution_status(SolutionStatus::NO_SOLUTION)
            {}
    };
}

#endif
