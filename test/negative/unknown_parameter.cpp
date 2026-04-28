// SPDX-License-Identifier: MIT
// Negative test: unknown parameter tag should fail to compile.

#include <nova/parameter/parameter.hpp>

#include <cstddef>
#include <tuple>

struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};

struct unknown_tag : nova::parameter::parameter_tag< unknown_tag >
{};

template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};

struct unknown_param : nova::parameter::flag_param< unknown_tag >
{};

// Only capacity_tag allowed
using allowed_tags = std::tuple< capacity_tag >;

template < typename... Params >
    requires nova::parameter::valid_parameters< allowed_tags, Params... >
struct my_struct
{};

// Should fail: unknown_param's tag not in allowed_tags
my_struct< capacity< 64 >, unknown_param > x;
