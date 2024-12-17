#include "shared_memory_communication.hpp"

#include <codecvt>      // for std::codecvt_utf8_utf16
#include "serialization.hpp"

using namespace boost::interprocess;

constexpr auto c_shared_memory_base_name = "ScaiIlpSolver";
constexpr auto c_num_shared_memory_name_trials = 10000;

namespace ilp_solver
{
    /***************************************
    * (De-) Serialization of solution data *
    ***************************************/
    static void serialize_result(Serializer* v_serializer, const ILPSolutionData& p_solution_data)
    {
        *v_serializer << p_solution_data.solution
                      << p_solution_data.objective
                      << p_solution_data.solution_status;
    }


    static void deserialize_result(Deserializer* v_deserializer, ILPSolutionData* r_solution_data)
    {
        *v_deserializer >> r_solution_data->solution
                        >> r_solution_data->objective
                        >> r_solution_data->solution_status;
    }


    /**********************************
    * (De-) Serialization of ILP data *
    **********************************/
    static void* serialize_ilp_data(Serializer* v_serializer, const ILPData& p_data, const ILPSolutionData& p_solution_data)
    {
        *v_serializer << p_data.matrix
                      << p_data.objective
                      << p_data.variable_lower
                      << p_data.variable_upper
                      << p_data.constraint_lower
                      << p_data.constraint_upper
                      << p_data.variable_type
                      << p_data.objective_sense
                      << p_data.start_solution
                      << p_data.num_threads
                      << p_data.deterministic
                      << p_data.log_level
                      << p_data.presolve
                      << p_data.max_seconds
                      << p_data.max_nodes
                      << p_data.max_solutions
                      << p_data.max_abs_gap
                      << p_data.max_rel_gap;

        auto result_address = v_serializer->current_address();

        serialize_result(v_serializer, p_solution_data);

        return result_address;
    }


    static void* deserialize_ilp_data(Deserializer* v_deserializer, ILPData* r_data)
    {
        *v_deserializer >> r_data->matrix
                        >> r_data->objective
                        >> r_data->variable_lower
                        >> r_data->variable_upper
                        >> r_data->constraint_lower
                        >> r_data->constraint_upper
                        >> r_data->variable_type
                        >> r_data->objective_sense
                        >> r_data->start_solution
                        >> r_data->num_threads
                        >> r_data->deterministic
                        >> r_data->log_level
                        >> r_data->presolve
                        >> r_data->max_seconds
                        >> r_data->max_nodes
                        >> r_data->max_solutions
                        >> r_data->max_abs_gap
                        >> r_data->max_rel_gap;

        return v_deserializer->current_address();
    }


    // To reserve space for the solution in the shared memory
    static ILPSolutionData dummy_solution(const ILPData& p_data)
    {
        ILPSolutionData dummy_solution_data(p_data.objective_sense);
        dummy_solution_data.solution.resize(p_data.variable_type.size());
        return dummy_solution_data;
    }


    static size_t determine_required_size(const ILPData& p_data)
    {
        Serializer serializer(nullptr);
        serialize_ilp_data(&serializer, p_data, dummy_solution(p_data));
        return serializer.required_bytes();
    }


    static void* serialize_ilp_data(void* p_address, const ILPData& p_data)
    {
        Serializer serializer(p_address);
        return serialize_ilp_data(&serializer, p_data, ILPSolutionData(p_data.objective_sense));
    }


    /******************************
    * Communication of the parent *
    ******************************/
    static windows_shared_memory* determine_free_shared_memory_name(size_t p_size, std::string* r_shared_memory_name)
    {
        windows_shared_memory* shared_memory = nullptr;
        for (auto trial = 1; trial <= c_num_shared_memory_name_trials; ++trial)
        {
            *r_shared_memory_name = c_shared_memory_base_name + std::to_string(trial);
            try
            {
                shared_memory = new windows_shared_memory(create_only, r_shared_memory_name->c_str(), read_write, p_size);
                break;
            }
            catch (const interprocess_exception& p_e)
            {
                if (p_e.get_error_code() != error_code_t::already_exists_error || trial == c_num_shared_memory_name_trials)
                    throw;
            }
        }
        return shared_memory;
    }


    CommunicationParent::CommunicationParent()
        : d_shared_memory(nullptr),
          d_mapped_region(nullptr),
          d_address(nullptr),
          d_result_address(nullptr)
        {}


    CommunicationParent::~CommunicationParent()
    {
        delete d_shared_memory;
        delete d_mapped_region;
    }


    std::string CommunicationParent::create_shared_memory(size_t p_size)
    {
        std::string shared_memory_name;
        d_shared_memory = determine_free_shared_memory_name(p_size, &shared_memory_name);
        d_mapped_region = new mapped_region(*d_shared_memory, read_write);
        d_address = d_mapped_region->get_address();
        return shared_memory_name;
    }


    std::string CommunicationParent::write_ilp_data(const ILPData& p_data)
    {
        const auto size = determine_required_size(p_data);
        const auto shared_memory_name = create_shared_memory(size);
        d_result_address = serialize_ilp_data(d_address, p_data);
        return shared_memory_name;
    }


    void CommunicationParent::read_solution_data(ILPSolutionData* r_solution_data)
    {
        Deserializer deserializer(d_result_address);
        deserialize_result(&deserializer, r_solution_data);
    }


    /*****************************
    * Communication of the child *
    *****************************/
    CommunicationChild::CommunicationChild(const std::string& p_shared_memory_name)
        : d_shared_memory(open_only, p_shared_memory_name.c_str(), read_write),
          d_mapped_region(d_shared_memory, read_write),
          d_address(d_mapped_region.get_address()),
          d_result_address(nullptr)
        {}


    void CommunicationChild::read_ilp_data(ILPData* r_data)
    {
        Deserializer deserializer(d_address);
        d_result_address = deserialize_ilp_data(&deserializer, r_data);
    }


    void CommunicationChild::write_solution_data(const ILPSolutionData& p_solution_data)
    {
        Serializer serializer(d_result_address);
        serialize_result(&serializer, p_solution_data);
    }


    /*********************************
    * Convert between UTF8 and UTF16 *
    *********************************/
#pragma warning(push)
#pragma warning(disable : 4996) // silence C++17 conformance warning
    std::wstring utf8_to_utf16(const std::string& p_utf8_string)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(p_utf8_string);
    }


    std::string utf16_to_utf8(const std::wstring& p_utf16_string)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(p_utf16_string);
    }
#pragma warning(pop)
}
