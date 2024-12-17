#pragma once

#include "ilp_solver_interface.hpp"


// The implementation serves to avoid redundant code duplication.
namespace ilp_solver
{
    enum class VariableType   { INTEGER, CONTINUOUS, BINARY };
    enum class ObjectiveSense { MINIMIZE, MAXIMIZE };


    // You may call this free function in the constructor of your fully implemented class.
    // Can not be called in the constructor of ILPSolverImpl since the virtual functions are not yet overridden.
    void set_default_parameters(ILPSolverInterface* p_solver);

    // This is the base class for any solver not using some kind of Osi-modeling or full interface.
    // It implements some methods of ILPSolverInterface by introducing fewer private virtual methods,
    // collecting multiple cases at once.
    class ILPSolverImpl : public ILPSolverInterface
    {
        public:
            void add_variable_boolean    (                                                                                double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_boolean    (                                       const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_boolean    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective,                                             const std::string& p_name = "") override;
            void add_variable_integer    (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_integer    (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_integer    (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (                                                                                double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (                                       const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;
            void add_variable_continuous (const std::vector<int>& p_row_indices, const std::vector<double>& p_row_values, double p_objective, double p_lower_bound, double p_upper_bound, const std::string& p_name = "") override;

            void add_constraint          (                                       const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint          (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound, double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_upper    (                                       const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_upper    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                       double p_upper_bound,                     const std::string& p_name = "") override;
            void add_constraint_lower    (                                       const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override;
            void add_constraint_lower    (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values, double p_lower_bound,                                           const std::string& p_name = "") override;
            void add_constraint_equality (                                       const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") override;
            void add_constraint_equality (const std::vector<int>& p_col_indices, const std::vector<double>& p_col_values,                                              double p_value,    const std::string& p_name = "") override;

            void minimize() override;
            void maximize() override;

        protected:
            ILPSolverImpl() = default;

        private:
            // If there is anything that needs to be done before a solve, overwrite prepare_impl.
            // It will be called before set_objective_sense_impl and solve_impl.
            // Useful e.g. for cached problems etc.
            // The default version does nothing.
            virtual void                      prepare_impl();
            virtual void                      add_variable_impl (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                                                                 const std::string& p_name = "", const std::vector<double>* p_row_values = nullptr,
                                                                 const std::vector<int>* p_row_indices = nullptr) = 0;
            virtual void                      add_constraint_impl (double p_lower_bound, double p_upper_bound,
                                                                   const std::vector<double>& p_col_values, const std::string& p_name = "",
                                                                   const std::vector<int>* p_col_indices = nullptr) = 0;
            virtual void                      solve_impl() = 0;
            virtual void                      set_objective_sense_impl(ObjectiveSense p_sense) = 0;
    };
}
