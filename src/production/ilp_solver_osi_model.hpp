#pragma once

#ifdef WITH_OSI

#include "ilp_solver_impl.hpp"

#include <CoinModel.hpp>

#include <string>
#include <vector>

class OsiSolverInterface;


namespace ilp_solver
{
    // Implements all methods from ILPSolverInterfaceImpl that can be realized
    // via the pure problem model of the OsiSolverInterface.
    // If your solver uses an OsiInterface to store the problem model,
    // but provides an additional interface on top of the OsiInterface
    // you may want to derive from this class.
    class ILPSolverOsiModel : public ILPSolverImpl
    {
        public:
            int  get_num_constraints() const override;
            int  get_num_variables  () const override;

            void print_mps_file     (const std::string& p_filename) override;
        protected:
            ILPSolverOsiModel() = default;

            void prepare_impl() override;

            CoinModel d_cache{};
            bool      d_cache_changed{ false };
        private:
            // Obtain a pointer to a solver fulfilling the OsiSolverInterface.
            virtual OsiSolverInterface* get_solver_osi_model() = 0;

            void add_variable_impl (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
                                   [[maybe_unused]] const std::string& p_name = "", OptionalValueArray p_row_values = {},
                                   OptionalIndexArray p_row_indices = {}) override;

            void add_constraint_impl (double p_lower_bound, double p_upper_bound,
                                      ValueArray p_col_values, [[maybe_unused]] const std::string& p_name = "",
                                      OptionalIndexArray p_col_indices = {}) override;

            // Helper object for dense -> sparse conversions.
            SparseVec d_sparse{};
    };
}

#endif
