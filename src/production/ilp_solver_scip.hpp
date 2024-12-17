#pragma once

#if WITH_SCIP == 1

#include "ilp_solver_impl.hpp"

#pragma comment(lib, "scip.lib")


namespace ilp_solver
{
    // Necessary forward declarations of structs and typedefs.
    struct  Scip;
    typedef Scip SCIP;
    struct  SCIP_Cons;
    typedef SCIP_Cons SCIP_CONS;
    struct  SCIP_Var;
    typedef SCIP_Var SCIP_VAR;
    enum    SCIP_Vartype : int;
    typedef SCIP_Vartype SCIP_VARTYPE;

    // Final Implementation of SCIP inside ScaiILP.
    class ILPSolverSCIP : public ILPSolverImpl
    {
    public:

        ILPSolverSCIP();
        ~ILPSolverSCIP();

        int get_num_constraints() const override;
        int get_num_variables()   const override;

        std::vector<double> get_solution()  const override;
        double              get_objective() const override;
        SolutionStatus      get_status()    const override;

        void                reset_solution()      override;

        void set_start_solution(const std::vector<double>& p_solution) override;

        void set_num_threads(int p_num_threads)           override;
        void set_deterministic_mode(bool p_deterministic) override;
        void set_log_level(int p_level)                   override;
        void set_presolve(bool p_presolve)                override;

        void set_max_seconds(double p_seconds)            override;
        void set_max_nodes(int p_nodes)                   override;
        void set_max_solutions(int p_solutions)           override;
        void set_max_abs_gap(double p_gap)                override;
        void set_max_rel_gap(double p_gap)                override;

        void print_mps_file(const std::string& p_path)    override;

    private:
        SCIP* d_scip;

        std::vector<SCIP_CONS*>   d_rows;
        std::vector<SCIP_VAR*>    d_cols;

        void set_objective_sense_impl(ObjectiveSense p_sense) override;
        void solve_impl() override;
        void add_variable_impl (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
            const std::string& p_name = "", const std::vector<double>* p_row_values = nullptr,
            const std::vector<int>* p_row_indices = nullptr) override;


        void add_constraint_impl (double p_lower_bound, double p_upper_bound,
            const std::vector<double>& p_col_values, const std::string& p_name = "",
            const std::vector<int>* p_col_indices = nullptr) override;
    };
}

#endif
