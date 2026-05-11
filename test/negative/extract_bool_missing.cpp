// SPDX-License-Identifier: MIT
// Negative test: extract_bool_v with missing tag should fail to compile.

#include <nova/parameter/parameter.hpp>

#include <cstddef>

struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};

struct fixed_tag : nova::parameter::parameter_tag< fixed_tag >
{};

template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};

// Should fail: fixed_tag not in pack — static_assert fires
constexpr bool val = nova::parameter::extract_bool_v< fixed_tag, capacity< 64 > >;
