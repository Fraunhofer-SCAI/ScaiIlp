#pragma once

#include <cassert>
#include <limits>
#include <utility>
#include <vector> // for std::size

template<typename Container>
constexpr int isize(const Container& p_container)
{
    const auto size{ std::size(p_container) };
    assert(std::cmp_less(size, std::numeric_limits<int>::max()));
    return static_cast<int>(size);
}
