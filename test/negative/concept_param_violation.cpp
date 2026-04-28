// SPDX-License-Identifier: MIT
// Negative test: concept_param with type not satisfying the constraint should fail.

#include <nova/parameter/parameter.hpp>

#include <cstddef>
#include <type_traits>

struct allocator_tag : nova::parameter::parameter_tag< allocator_tag >
{};

// Constraint traits: type must be integral
template < typename T >
using must_be_integral = std::bool_constant< std::is_integral_v< T > >;

// concept_param requiring integral type
template < typename T >
struct integral_only : nova::parameter::concept_param< allocator_tag, T, must_be_integral >
{};

// Should fail: double does not satisfy must_be_integral
integral_only< double > x;
