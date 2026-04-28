// SPDX-License-Identifier: MIT
// Negative test: missing required parameter should fail to compile.

#include <nova/parameter/parameter.hpp>

#include <cstddef>
#include <tuple>

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

using allowed_tags  = std::tuple< capacity_tag, fixed_tag >;
using required_tags = std::tuple< capacity_tag >; // capacity is required

template < typename... Params >
    requires nova::parameter::valid_parameters< allowed_tags, Params... >
             && nova::parameter::required_parameters< required_tags, Params... >
struct my_struct
{};

// Should fail: capacity is required but not provided
my_struct< fixed_sized< true > > x;
