#pragma once

#include "ilp_data.hpp"
#include "ilp_solver_impl.hpp"

#include <functional>
#include <string>

namespace ilp_solver
{

// Stores all information about the ILP and the solver.
// Is used to create a stub.
class ILPSolverCollect : public ILPSolverImpl
{
public:
    int  get_num_constraints() const override;
    int  get_num_variables  () const override;

    // The file name is converted to boost::filesystem::path internally.
    // You may set a locale for boost::filesystem, such that your desired encoding is used.
    // Note in particular the convenient boost::nowide::nowide_filesystem.
    void print_mps_file(const std::string& p_filename) override;
protected:
    ILPSolverCollect();

    ILPData d_ilp_data;

private:

    void add_variable_impl(VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                           const std::string& p_name = "", OptionalValueArray p_row_values = {},
                           OptionalIndexArray p_row_indices = {}) override;

    void add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                             const std::string& p_name = "", OptionalIndexArray p_col_indices = {}) override;
    void set_objective_sense_impl(ObjectiveSense p_sense) override;

    void set_start_solution     (ValueArray p_solution) override;

    void set_num_threads        (int p_num_threads)    override;
    void set_deterministic_mode (bool p_deterministic) override;
    void set_log_level          (int p_level)          override;
    void set_presolve           (bool p_presolve)      override;

    void set_max_seconds_impl   (double p_seconds)     override;
    void set_max_nodes          (int p_nodes)          override;
    void set_max_solutions      (int p_solutions)      override;
    void set_max_abs_gap        (double p_gap)         override;
    void set_max_rel_gap        (double p_gap)         override;
    void set_cutoff             (double p_cutoff)      override;

    void set_interim_results    (std::function<void(ILPSolutionData*)>) override{ /* Not yet implemented. */ }
};

} // namespace ilp_solver
