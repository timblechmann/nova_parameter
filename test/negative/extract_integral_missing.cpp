// SPDX-License-Identifier: MIT
// Negative test: extract_integral_v with missing tag should fail to compile.

#include <nova/parameter/parameter.hpp>

#include <cstddef>

struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};

struct fixed_tag : nova::parameter::parameter_tag< fixed_tag >
{};

template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};

template < bool V >
struct fixed_sized : nova::parameter::bool_param< fixed_tag, V >
{};

// Should fail: capacity_tag not in pack — static_assert fires
constexpr std::size_t val = nova::parameter::extract_integral_v< capacity_tag, std::size_t, fixed_sized< true > >;
