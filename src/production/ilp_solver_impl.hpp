#pragma once

#include "ilp_solver_interface.hpp"

#include <optional>
#include <string>
#include <vector>

// The implementation serves to avoid redundant code duplication.
namespace ilp_solver
{

using OptionalIndexArray = std::optional<IndexArray>;
using OptionalValueArray = std::optional<ValueArray>;
enum class VariableType   { INTEGER, CONTINUOUS, BINARY };
enum class ObjectiveSense { MINIMIZE, MAXIMIZE };


// You may call this free function in the constructor of your fully implemented class.
// Can not be called in the constructor of ILPSolverImpl since the virtual functions are not yet overridden.
void set_default_parameters(ILPSolverInterface* p_solver);


// Convenience function to replace all spaces by '_' in the given string.
// Some solvers print the names of constraints and variables to MPS files.
// There, spaces are problematic, so implementations may use this function
// to sanitize the names before passing them to the solver.
std::string replace_spaces(const std::string& p_name);


// Helper struct to be used in implementations that have to convert dense vectors to sparse vectors.
class SparseVec
{
public:
    // Set d_indices and d_values to correspond to the given dense values.
    void        init_from_dense(ValueArray p_dense_values);
    auto        size() const { return d_indices.size(); }
    const auto& indices() const { return d_indices; }
    const auto& values() const { return d_values; }

private:
    std::vector<int>    d_indices{};
    std::vector<double> d_values{};
};


// This is the base class for any solver not using some kind of Osi-modeling or full interface.
// It implements some methods of ILPSolverInterface by introducing fewer private virtual methods,
// collecting multiple cases at once.
class ILPSolverImpl : public ILPSolverInterface
{
public:
    void add_variable_boolean    (                                                   double p_objective,                                             const std::string& p_name = "") override final;
    void add_variable_boolean    (                          ValueArray p_row_values, double p_objective,                                             const std::string& p_name = "") override final;
    void add_variable_boolean    (IndexArray p_row_indices, ValueArray p_row_values, double p_objective,                                             const std::string& p_name = "") override final;
    void add_variable_integer    (                                                   double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;
    void add_variable_integer    (                          ValueArray p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;
    void add_variable_integer    (IndexArray p_row_indices, ValueArray p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;
    void add_variable_continuous (                                                   double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;
    void add_variable_continuous (                          ValueArray p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;
    void add_variable_continuous (IndexArray p_row_indices, ValueArray p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override final;

    void add_constraint          (                          ValueArray p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override final;
    void add_constraint          (IndexArray p_col_indices, ValueArray p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override final;
    void add_constraint_upper    (                          ValueArray p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override final;
    void add_constraint_upper    (IndexArray p_col_indices, ValueArray p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override final;
    void add_constraint_lower    (                          ValueArray p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override final;
    void add_constraint_lower    (IndexArray p_col_indices, ValueArray p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override final;
    void add_constraint_equality (                          ValueArray p_col_values,                                              double p_value,    const std::string& p_name = "") override final;
    void add_constraint_equality (IndexArray p_col_indices, ValueArray p_col_values,                                              double p_value,    const std::string& p_name = "") override final;

    void minimize() override final;
    void maximize() override final;

    void set_max_seconds(double p_seconds) override final;
protected:
    ILPSolverImpl() = default;
    double d_max_seconds{}; // Stored separately because solver input may transform the seconds.

private:
    // If there is anything that needs to be done before a solve, overwrite prepare_impl.
    // It will be called before set_objective_sense_impl and solve_impl.
    // Useful e.g. for cached problems etc.
    // The default version does nothing.
    virtual void prepare_impl();
    virtual void add_variable_impl(VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                                   const std::string& p_name = "", OptionalValueArray p_row_values = {},
                                   OptionalIndexArray p_row_indices = {})                                   = 0;
    virtual void add_constraint_impl(double p_lower_bound, double p_upper_bound, ValueArray p_col_values,
                                     const std::string& p_name = "", OptionalIndexArray p_col_indices = {}) = 0;
    virtual void solve_impl()                                                                               = 0;
    virtual void set_objective_sense_impl(ObjectiveSense p_sense)                                           = 0;
    virtual void set_max_seconds_impl(double p_seconds)                                                     = 0;
};

} // namespace ilp_solver
