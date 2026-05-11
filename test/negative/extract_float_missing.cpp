// SPDX-License-Identifier: MIT
// Negative test: extract_float_v with missing tag should fail to compile.

#include <nova/parameter/parameter.hpp>

struct threshold_tag : nova::parameter::parameter_tag< threshold_tag >
{};

struct other_tag : nova::parameter::parameter_tag< other_tag >
{};

template < float F >
struct threshold : nova::parameter::float_param< threshold_tag, F >
{};

template < bool V >
struct flag : nova::parameter::bool_param< other_tag, V >
{};

// Should fail: threshold_tag not in pack — static_assert fires
constexpr float val = nova::parameter::extract_float_v< threshold_tag, flag< true > >;
