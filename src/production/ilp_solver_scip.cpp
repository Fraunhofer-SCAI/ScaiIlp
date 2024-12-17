#ifdef WITH_SCIP

#include "ilp_solver_scip.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cassert>
#include <memory>

namespace ilp_solver
{
    #include <scip/scip.h>
    #include <scip/scipdefplugins.h>

    namespace
    {
        // A template instead of the macro expansion to handle return codes.
        // If the SCIP function call was unsuccessful, throw an error.
        template<typename F, typename... Args>
        static __forceinline void call_scip(F p_f, Args... p_args)
        {
            auto retcode = p_f(p_args...);
            switch (retcode)
            {
                case SCIP_OKAY:               break;
                case SCIP_ERROR:              throw std::runtime_error("SCIP produced an unspecified error.");
                case SCIP_NOMEMORY:           throw std::runtime_error("SCIP has insufficient memory.");
                case SCIP_READERROR:          throw std::runtime_error("SCIP could not read data.");
                case SCIP_WRITEERROR:         throw std::runtime_error("SCIP could not write data.");
                case SCIP_NOFILE:             throw std::runtime_error("SCIP could not read file.");
                case SCIP_FILECREATEERROR:    throw std::runtime_error("SCIP could not write file.");
                case SCIP_LPERROR:            throw std::runtime_error("SCIP produced error in LP solve.");
                case SCIP_NOPROBLEM:          throw std::runtime_error("SCIP had no problem to solve.");
                case SCIP_INVALIDCALL:        throw std::runtime_error("SCIP tried to call a method that was invalid at this time.");
                case SCIP_INVALIDDATA:        throw std::runtime_error("SCIP tried to call a method with invalid data.");
                case SCIP_INVALIDRESULT:      throw std::runtime_error("SCIP method returned an invalid result code.");
                case SCIP_PLUGINNOTFOUND:     throw std::runtime_error("SCIP could not find a required plugin.");
                case SCIP_PARAMETERUNKNOWN:   throw std::runtime_error("SCIP could not find a parameter of the given name.");
                case SCIP_PARAMETERWRONGTYPE: throw std::runtime_error("SCIP parameter had an unexpected type.");
                case SCIP_PARAMETERWRONGVAL:  throw std::runtime_error("SCIP tried to set a parameter to an invalid value.");
                case SCIP_KEYALREADYEXISTING: throw std::runtime_error("SCIP tried to insert an already existing key into the table.");
                case SCIP_MAXDEPTHLEVEL:      throw std::runtime_error("SCIP exceeded the maximal branching depth level.");
                case SCIP_BRANCHERROR:        throw std::runtime_error("SCIP could not perform the branching.");
                default:                      throw std::runtime_error("SCIP produced an unknown error.");
            }
        };


        // RAII wrapper for SCIP_SOL.
        auto create_scoped_solution(SCIP* v_scip)
        {
            SCIP_SOL* sol;
            call_scip(SCIPcreateSol, v_scip, &sol, nullptr);
            auto deleter = [v_scip](SCIP_SOL* v_sol) { SCIPfreeSol(v_scip, &v_sol); };
            return std::unique_ptr<SCIP_SOL, decltype(deleter)>(sol, deleter);
        }
    } // namespace


    ILPSolverSCIP::ILPSolverSCIP()
    {
        call_scip(SCIPcreate, &d_scip);
        call_scip(SCIPincludeDefaultPlugins, d_scip);

        // All the nullptr's are possible User-data.
        call_scip(SCIPcreateProb, d_scip, "problem", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        call_scip(SCIPsetObjsense, d_scip, SCIP_OBJSENSE_MINIMIZE); // Needs a start objective sense.
        set_default_parameters(this);
    }


    ILPSolverSCIP::~ILPSolverSCIP()
    {
        // Variables and Constraints need to be released separately.
        // Do not use `call_scip` and ignore errors so that this destructor does not throw.
        for (auto& p : d_cols)
            SCIPreleaseVar(d_scip, &p);
        for (auto& p : d_rows)
            SCIPreleaseCons(d_scip, &p);
        SCIPfree(&d_scip);
    }


    int ILPSolverSCIP::get_num_constraints() const
    {
        return isize(d_rows);
    }


    int ILPSolverSCIP::get_num_variables() const
    {
        return isize(d_cols);
    }


    std::vector<double> ILPSolverSCIP::get_solution()  const
    {
        std::vector<double> res;
        if (get_status() != SolutionStatus::SUBOPTIMAL && get_status() != SolutionStatus::PROVEN_OPTIMAL)
            return res;

        res.reserve(d_cols.size());

        // Obtain the value for each variable in the current best solution.
        auto sol = SCIPgetBestSol(d_scip);
        for (auto& s : d_cols)
            res.push_back(SCIPgetSolVal(d_scip, sol, s));

        return res;
    }


    std::vector<double> ILPSolverSCIP::get_dual_sol() const
    {
        std::vector<double> res = {};
        if (!SCIPisDualSolAvailable(d_scip, true))
            return res;

        res.reserve(d_rows.size());

        // Obtain the dual value for each constraint in the current best solution.
        for (const auto& row : d_rows)
        {
            double x;
            SCIPgetDualSolVal(d_scip, row, &x, NULL);
            res.push_back(x);
        }

        return res;
    }


    double ILPSolverSCIP::get_objective() const
    {
        // The current primal bound is the best objective value attained.
        return SCIPgetPrimalbound(d_scip);
    }


    SolutionStatus ILPSolverSCIP::get_status() const
    {
        int n = 0; // There are null-pointer accesses if called in the wrong stage, which happens if reset.
        switch (SCIPgetStage(d_scip))
        {
        case SCIP_STAGE_TRANSFORMED:  [[fallthrough]];
        case SCIP_STAGE_INITPRESOLVE: [[fallthrough]];
        case SCIP_STAGE_PRESOLVING:   [[fallthrough]];
        case SCIP_STAGE_EXITPRESOLVE: [[fallthrough]];
        case SCIP_STAGE_PRESOLVED:    [[fallthrough]];
        case SCIP_STAGE_INITSOLVE:    [[fallthrough]];
        case SCIP_STAGE_SOLVING:      [[fallthrough]];
        case SCIP_STAGE_SOLVED:       [[fallthrough]];
        case SCIP_STAGE_EXITSOLVE:
            n = static_cast<int>(SCIPgetNSolsFound(d_scip));
        }

        SolutionStatus ret = (n > 0) ? SolutionStatus::SUBOPTIMAL : SolutionStatus::NO_SOLUTION;

        // Handle all possible status values. Almost all will be reduced to SUBOPTIMAL or NO_SOLUTION.
        switch (SCIPgetStatus(d_scip))
        {
        case SCIP_STATUS_OPTIMAL:    return SolutionStatus::PROVEN_OPTIMAL;
        case SCIP_STATUS_INFEASIBLE: return SolutionStatus::PROVEN_INFEASIBLE;
        case SCIP_STATUS_UNBOUNDED:  return SolutionStatus::PROVEN_UNBOUNDED;
        case SCIP_STATUS_UNKNOWN:        [[fallthrough]];
        case SCIP_STATUS_INFORUNBD:      [[fallthrough]];
        case SCIP_STATUS_NODELIMIT:      [[fallthrough]];
        case SCIP_STATUS_TOTALNODELIMIT: [[fallthrough]];
        case SCIP_STATUS_STALLNODELIMIT: [[fallthrough]];
        case SCIP_STATUS_TIMELIMIT:      [[fallthrough]];
        case SCIP_STATUS_MEMLIMIT:       [[fallthrough]];
        case SCIP_STATUS_GAPLIMIT:       [[fallthrough]];
        case SCIP_STATUS_SOLLIMIT:       [[fallthrough]];
        case SCIP_STATUS_BESTSOLLIMIT:   [[fallthrough]];
        case SCIP_STATUS_RESTARTLIMIT:   [[fallthrough]];
        case SCIP_STATUS_USERINTERRUPT:  [[fallthrough]];
        case SCIP_STATUS_TERMINATE:      [[fallthrough]];
        default:                     return ret;
        }
    }


    void ILPSolverSCIP::reset_solution()
    {
        // freeTransform does keep the found solutions in the starting solution storage.
        // To hack this, we set the number of stored solutions to 0 before we free, then reset it.
        auto tmp{0};
        call_scip(SCIPgetIntParam, d_scip, "limits/maxorigsol", &tmp);
        call_scip(SCIPsetIntParam, d_scip, "limits/maxorigsol", 0);

        // "frees all solution process data including presolving and transformed problem,
        //  only original problem is kept"
        // This may be overkill, but I have not found another method that actually reset solution data.
        // Seems unnecessarily slow though.
        call_scip(SCIPfreeTransform, d_scip);

        call_scip(SCIPsetIntParam, d_scip, "limits/maxorigsol", tmp);
    }


    void ILPSolverSCIP::set_start_solution(ValueArray p_solution)
    {
        assert(p_solution.size() == d_cols.size());

        // May not be able to set the solution after the problem has been solved or transformed.
        if (SCIPgetStage(d_scip) == SCIP_STAGE_SOLVED)
            reset_solution();

        // SCIP uses a double*, not a const double*.
        // Internally, SCIP calls a single-variable setter for every variable with a by-value pass of the corresponding double,
        // so the const_cast should not violate actual const-ness.
        // Sadly, it is not avoidable since SCIP is not const-correct. (SCIP 6.0)
        auto sol = create_scoped_solution(d_scip);
        call_scip(SCIPsetSolVals, d_scip, sol.get(), isize(d_cols), d_cols.data(), const_cast<double*>(p_solution.data()));
        SCIP_Bool feasible = TRUE;
        call_scip(SCIPcheckSol, d_scip, sol.get(), FALSE /*print reason*/, TRUE /*completely*/, TRUE /*check bounds*/,
                  TRUE /*check integrality*/, TRUE /*check LP rows*/, &feasible);
        if (feasible != TRUE)
            throw InvalidStartSolutionException();
        SCIP_Bool ignored{false};
        call_scip(SCIPaddSol, d_scip, sol.get(), &ignored);
    }


    void ILPSolverSCIP::set_num_threads(int p_num_threads)
    {
        // Possibly does nothing if not using FiberSCIP or some other parallelization method.
        call_scip(SCIPsetIntParam, d_scip, "parallel/maxnthreads", p_num_threads);

        // Number of threads for solving the LP, 0 is automatic.
        // 64 is the explicit maximal number.
        p_num_threads = std::clamp(p_num_threads, 0, 64);
        call_scip(SCIPsetIntParam, d_scip, "lp/threads", p_num_threads);
    }


    void ILPSolverSCIP::set_deterministic_mode(bool p_deterministic)
    {
        // Possibly does nothing if not using FiberSCIP or some other parallelization method.
        call_scip(SCIPsetIntParam, d_scip, "parallel/mode", p_deterministic);
    }


    void ILPSolverSCIP::set_log_level(int p_level)
    {
        p_level = std::clamp(p_level, 0, 5); // Minimum level is 0 (no output), maximum level is 5.
        call_scip(SCIPsetIntParam, d_scip, "display/verblevel", p_level);
        if (p_level == 0)
        {
            SCIPsetMessagehdlrQuiet(d_scip, true);
            SCIPmessageSetErrorPrinting(nullptr, nullptr); // suppress error printing.
        }
        else
        {
            SCIPsetMessagehdlrQuiet(d_scip, false);
            SCIPmessageSetErrorPrintingDefault(); // print errors to cerr.
        }
    }


    void ILPSolverSCIP::set_presolve(bool p_presolve)
    {
        call_scip(SCIPsetBoolParam, d_scip, "lp/presolving", p_presolve);
        call_scip(SCIPsetIntParam, d_scip, "presolving/maxrounds", (p_presolve) ? -1 : 0); // -1 is default, 0 is off.

        // Disable/Enable Heuristics.
        // #TODO: What does this have to do with presolve?
        if (p_presolve) call_scip(SCIPsetHeuristics, d_scip, SCIP_PARAMSETTING_DEFAULT, TRUE);
        else            call_scip(SCIPsetHeuristics, d_scip, SCIP_PARAMSETTING_OFF,     TRUE);
    }


    void ILPSolverSCIP::set_max_seconds_impl(double p_seconds)
    {
        p_seconds = std::clamp(p_seconds, 0., 1e20); // SCIP Maximum.
        call_scip(SCIPsetRealParam, d_scip, "limits/time", p_seconds);
    }


    void ILPSolverSCIP::set_max_nodes(int p_nodes)
    {
        p_nodes = (p_nodes == std::numeric_limits<int>::max()) ? -1 : p_nodes; // -1 is no limit.
        call_scip(SCIPsetLongintParam, d_scip, "limits/totalnodes", p_nodes); // total nodes including restarts.
    }


    void ILPSolverSCIP::set_max_solutions(int p_solutions)
    {
        p_solutions = (p_solutions == std::numeric_limits<int>::max()) ? -1 : p_solutions; // -1 is no limit.
        call_scip(SCIPsetIntParam, d_scip, "limits/solutions", p_solutions);
    }


    void ILPSolverSCIP::set_max_abs_gap(double p_gap)
    {
        p_gap = std::max(0., p_gap); // |primalbound - dualbound|
        call_scip(SCIPsetRealParam, d_scip, "limits/absgap", p_gap);
    }


    void ILPSolverSCIP::set_max_rel_gap(double p_gap)
    {
        p_gap = std::max(0., p_gap); // |primalbound - dualbound| / |min(primalbound, dualbound)|
        call_scip(SCIPsetRealParam, d_scip, "limits/gap", p_gap);
    }


    void ILPSolverSCIP::set_cutoff(double p_cutoff)
    {
        // Only set cutoff if intended. Otherwise stick to the Scip default.
        if (p_cutoff != c_default_cutoff)
            call_scip(SCIPsetObjlimit, d_scip, p_cutoff);
    }


    void ILPSolverSCIP::print_mps_file(const std::string& p_path)
    {
        // uses the extension of p_path, so this has to be ".mps".
        assert ( p_path.substr(p_path.rfind('.'), std::string::npos) == ".mps" );
        call_scip(SCIPwriteOrigProblem, d_scip, p_path.c_str(), nullptr, FALSE); // The bool is "generic names"
    }


    void ILPSolverSCIP::set_objective_sense_impl(ObjectiveSense p_sense)
    {
        auto sense{ (p_sense == ObjectiveSense::MINIMIZE) ? SCIP_OBJSENSE_MINIMIZE : SCIP_OBJSENSE_MAXIMIZE };
        call_scip(SCIPsetObjsense, d_scip, sense);
    }


    void ILPSolverSCIP::solve_impl()
    {
        call_scip(SCIPsolve, d_scip);
    }


    void ILPSolverSCIP::add_variable_impl (VariableType p_type, double p_objective, double p_lower_bound, double p_upper_bound,
        const std::string& p_name, OptionalValueArray p_row_values, OptionalIndexArray p_row_indices)
    {
        SCIP_VAR*    var;
        auto         name = replace_spaces(p_name);
        SCIP_VARTYPE type = (p_type == VariableType::INTEGER)    ? SCIP_VARTYPE_INTEGER
                          : (p_type == VariableType::CONTINUOUS) ? SCIP_VARTYPE_CONTINUOUS : SCIP_VARTYPE_BINARY;
        // Creates a variable of type p_type with corresponding objectives and bounds.
        // The parameters after p_type are:
        //     initial:   true  (the column belonging to var is present in the initial root LP.)
        //     removable: false (the column belonging to var is not removable from the LP.)
        //     User Data pointers.
        call_scip(SCIPcreateVar, d_scip, &var, name.c_str(), p_lower_bound, p_upper_bound, p_objective, type, TRUE,
                  FALSE, nullptr, nullptr, nullptr, nullptr, nullptr);
        call_scip( SCIPaddVar, d_scip, var );
        d_cols.push_back(var); // We need to store the variables separately to access them later on.

        auto n = isize(d_rows); // num_constraints.

        if (p_row_values) // If we have coefficients given...
        {
            if (p_row_indices) // and indices...
            {
                assert( p_row_values->size() == p_row_indices->size() );
                // Add the corresponding coefficient to every constraint indexed.
                for (auto i : *p_row_indices)
                {
                    assert( i < n );
                    call_scip( SCIPaddCoefLinear, d_scip, d_rows[i], var, (*p_row_values)[i] );
                }
            }
            else
            {
                assert( p_row_values->size() >= d_rows.size() );
                // Add the corresponding coefficient to every constraint in the problem.
                for (int i = 0; i < n; i++)
                {
                    call_scip( SCIPaddCoefLinear, d_scip, d_rows[i], var, (*p_row_values)[i] );
                }
            }
        }
    }


    void ILPSolverSCIP::add_constraint_impl (double p_lower_bound, double p_upper_bound,
        ValueArray p_col_values, const std::string& p_name, OptionalIndexArray p_col_indices)
    {
        SCIP_CONS*             cons;
        SCIP_VAR**             vars;
        std::vector<SCIP_VAR*> tmp;
        int                    size{0};
        auto                   name = replace_spaces(p_name);

        // If we have no indices given, we need to have a coefficient for every variable in the problem.
        if (!p_col_indices)
        {
            assert(p_col_values.size() >= d_cols.size());
            vars = d_cols.data();
            size = get_num_variables();
        }
        else
        {
            // Otherwise we need to create a vector of the correct variables given by their indices.
            assert(p_col_values.size() >= p_col_indices->size());
            tmp.reserve(p_col_indices->size());
            for (int i : *p_col_indices)
            {
                assert(i < get_num_variables());
                tmp.push_back(d_cols[i]);
            }
            vars = tmp.data();
            size = isize(tmp);
        }

        // SCIP uses a double*, not a const double*, but ScaiILP demands a const std::vector<double>&.
        // Internally, SCIP copies the buffer, so the const_cast should not violate actual const-ness.
        // Sadly, it is not avoidable since SCIP is not const-correct. (SCIP 6.0)
        // The parameters after p_rhs are:
        //    initial:        true  (the relaxed constraint is in the initial LP.)
        //    separate:       true  (the constraint should be separated during LP processing.)
        //    enforce:        true  (the constraint is enforced during node processing.)
        //    check:          true  (the constraint is checked for feasibility.)
        //    propagate:      true  (the constraint is propagated during node processing.)
        //    local:          false (the constraint is valid globally.)
        //    modifiable:     false (the constraint is not subject to column generation.)
        //    dynamic:        false (the constraint is not subject to aging.)
        //    removable:      false (the constraint may not be removed during aging or cleanup.)
        //    stickingatnode: false (the constraint should not be kept at the node where it was added.)
        call_scip(SCIPcreateConsLinear, d_scip, &cons, name.c_str(), size, vars, const_cast<double*>(p_col_values.data()),
                  p_lower_bound, p_upper_bound, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE);
        call_scip(SCIPaddCons, d_scip, cons);
        d_rows.push_back(cons);
    }
}

#endif
