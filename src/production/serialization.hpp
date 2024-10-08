#pragma once

#include "utility.hpp"

#include <boost/utility.hpp>
#include <type_traits>
#include <cstddef>
#include <span>
#include <vector>

template<typename T>
concept POD = std::is_pod_v<T>;

/**********************************
* Serializes plain old data types *
* (POD types) and vectors         *
**********************************/
class Serializer
{
    public:
        // p_address == nullptr simulates serialization. This allows calculating
        // the amount of required bytes without writing the data to a temporary
        // memory location.
        explicit Serializer(void* p_address)
            : d_start_address(static_cast<char*>(p_address)),
              d_current_address(static_cast<char*>(p_address)),
              d_simulate(d_start_address == nullptr ? true : false)
            {}

        // copy constructor and assignment operator are not allowed (due to const members)
        Serializer(const Serializer&) = delete;
        Serializer& operator= (const Serializer&) = delete;

        std::size_t required_bytes() const
        {
            return d_current_address - d_start_address;
        }

        void* current_address() const { return d_current_address; }

        template<POD T>      void serialize(const T& p_value);
        template<POD T>      void serialize(const std::vector<T>& p_vector);
        template<typename T> void serialize(const std::vector<std::vector<T>>& p_vector_of_vectors);

    private:
        const char* d_start_address;
        char* d_current_address;
        const bool d_simulate;
};


/************************************
* Deserializes plain old data types *
* (POD types) and vectors           *
************************************/
class Deserializer
{
    public:
        explicit Deserializer(void* p_address) : d_current_address(static_cast<char*>(p_address)) {}

        void* current_address() const { return d_current_address; }

        template<POD T>      void deserialize(T& r_value);
        template<POD T>      void deserialize(std::vector<T>& r_vector);
        template<POD T>      void deserialize(std::span<T>& r_span);
        template<POD T>      void deserialize(std::vector<std::span<T>>& r_vector_of_spans);
        template<typename T> void deserialize(std::vector<std::vector<T>>& r_vector_of_vectors);

    private:
        char* d_current_address;
};

/*****************************************************
* Embedded DSL for serialization and deserialization *
*****************************************************/

// Serialization: serializer << xyz
// ================================
template<typename Serializable>
Serializer& operator<<(Serializer& p_serializer, const Serializable& p_serializable)
{
    p_serializer.serialize(p_serializable);
    return p_serializer;
}


// Deserialization: deserializer >> xyz
// ====================================
template<typename Deserializable>
Deserializer& operator>>(Deserializer& p_deserializer, Deserializable& p_deserializable)
{
    p_deserializer.deserialize(p_deserializable);
    return p_deserializer;
}


/*****************
* Implementation *
*****************/


// Increases p_num_bytes to the next multiple of sizeof(std::max_align_t)
constexpr std::size_t num_bytes_with_padding(std::size_t p_num_bytes)
{
    const auto alignment = sizeof(std::max_align_t);
    --p_num_bytes;
    return p_num_bytes - (p_num_bytes % alignment) + alignment;
}

static_assert(num_bytes_with_padding(4) == sizeof(std::max_align_t));
static_assert(num_bytes_with_padding(8) == sizeof(std::max_align_t));


// (De-) Serialization of a POD type
// =================================
template<POD T>
void Serializer::serialize(const T& p_value)
{
    const auto num_bytes = sizeof(T);
    if (!d_simulate)
    {
        auto address = static_cast<T*>(static_cast<void*>(d_current_address));
        *address = p_value;
    }
    d_current_address += num_bytes_with_padding(num_bytes);
}


template<POD T>
void Deserializer::deserialize(T& r_value)
{
    const auto num_bytes = sizeof(T);
    auto address = static_cast<T*>(static_cast<void*>(d_current_address));
    r_value = *address;
    d_current_address += num_bytes_with_padding(num_bytes);
}


// (De-) Serialization of a POD type vector
// ========================================
template<POD T>
void Serializer::serialize(const std::vector<T>& p_vector)
{
    const auto size = isize(p_vector);
    serialize(size);
    const auto num_bytes = size*sizeof(T);
    if (!d_simulate)
        std::memcpy(d_current_address, p_vector.data(), num_bytes);
    d_current_address += num_bytes_with_padding(num_bytes);
}


template<POD T>
void Deserializer::deserialize(std::vector<T>& r_vector)
{
    int size;
    deserialize(size);
    const auto num_bytes = size*sizeof(T);
    r_vector.resize(size);
    std::memcpy(r_vector.data(), d_current_address, num_bytes);
    d_current_address += num_bytes_with_padding(num_bytes);
}


template<POD T>
void Deserializer::deserialize(std::span<T>& r_span)
{
    int size;
    deserialize(size);
    const auto num_bytes = size*sizeof(T);
    const auto start     = static_cast<T*>(static_cast<void*>(d_current_address));
    r_span               = std::span<T>(start, size);
    d_current_address += num_bytes_with_padding(num_bytes);
}


// (De-) Serialization of a vector of vectors
// ==========================================
template<typename T>
void Serializer::serialize(const std::vector<std::vector<T>>& p_vector_of_vectors)
{
    const auto size = isize(p_vector_of_vectors);
    serialize(size);
    for (const auto& vector: p_vector_of_vectors)
        serialize(vector);
}


template<typename T>
void Deserializer::deserialize(std::vector<std::vector<T>>& r_vector_of_vectors)
{
    int size;
    deserialize(size);
    r_vector_of_vectors.resize(size);
    for (auto& vector: r_vector_of_vectors)
        deserialize(vector);
}


template<POD T>
void Deserializer::deserialize(std::vector<std::span<T>>& r_vector_of_spans)
{
    int size;
    deserialize(size);
    r_vector_of_spans = std::vector<std::span<T>>(size);
    for (auto& span : r_vector_of_spans)
        deserialize(span);
}
