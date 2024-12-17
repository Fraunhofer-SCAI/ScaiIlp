#ifndef _SHARED_MEMORY_COMMUNICATION_HPP
#define _SHARED_MEMORY_COMMUNICATION_HPP

#include "ilp_data.hpp"

#include <memory>
#include <string>
#include <vector>

#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace ilp_solver
{
    class CommunicationParent
    {
        public:
            CommunicationParent();
            ~CommunicationParent();

            // Returns the name of the shared memory segment the data has been written to
            std::string write_ilp_data(const ILPData& p_data);
            void read_solution_data(ILPSolutionData* r_solution_data);

        private:
            boost::interprocess::windows_shared_memory* d_shared_memory;
            boost::interprocess::mapped_region* d_mapped_region;

            // non-owned pointer; do not delete
            void* d_address;
            void* d_result_address;

            std::string create_shared_memory(size_t p_size);
    };


    class CommunicationChild
    {
        public:
            explicit CommunicationChild(const std::string& p_shared_memory_name);

            void read_ilp_data(ILPData* r_data);
            void write_solution_data(const ILPSolutionData& p_solution_data);

        private:
            const boost::interprocess::windows_shared_memory d_shared_memory;
            const boost::interprocess::mapped_region d_mapped_region;

            // non-owned pointer; do not delete
            void* const d_address;
            void* d_result_address;
    };
}

#endif
