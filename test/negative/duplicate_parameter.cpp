// SPDX-License-Identifier: MIT
// Negative test: duplicate parameter tags should fail to compile.

#include <nova/parameter/parameter.hpp>

#include <cstddef>
#include <tuple>

struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};

template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};

using allowed_tags = std::tuple< capacity_tag >;

template < typename... Params >
    requires nova::parameter::valid_parameters< allowed_tags, Params... >
struct my_struct
{};

// Should fail: duplicate capacity
my_struct< capacity< 64 >, capacity< 128 > > x;
