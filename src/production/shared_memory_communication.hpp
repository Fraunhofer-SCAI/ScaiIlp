#pragma once

#include "ilp_data.hpp"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/windows_shared_memory.hpp>
#include <memory>
#include <string>


namespace ilp_solver
{

class CommunicationParent
{
public:
    // Returns the name of the shared memory segment the data has been written to
    std::string write_ilp_data(const ILPData& p_data);
    void        read_solution_data(ILPSolutionData* r_solution_data);

private:
    std::unique_ptr<boost::interprocess::windows_shared_memory> d_shared_memory{};
    std::unique_ptr<boost::interprocess::mapped_region>         d_mapped_region{};

    // non-owned pointer; do not delete
    void* d_address{};
    void* d_result_address{};

    std::string create_shared_memory(size_t p_size);
};


class CommunicationChild
{
public:
    explicit CommunicationChild(const std::string& p_shared_memory_name);
    CommunicationChild(const CommunicationChild&) = delete;
    CommunicationChild(CommunicationChild&&)      = delete;

    ILPDataView read_ilp_data();
    void        write_solution_data(const ILPSolutionData& p_solution_data);

private:
    const boost::interprocess::windows_shared_memory d_shared_memory;
    const boost::interprocess::mapped_region         d_mapped_region;

    // non-owned pointer; do not delete
    void* const d_address;
    void*       d_result_address;
};

} // namespace ilp_solver
