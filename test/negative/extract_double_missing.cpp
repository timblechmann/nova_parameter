// SPDX-License-Identifier: MIT
// Negative test: extract_double_v with missing tag should fail to compile.

#include <nova/parameter/parameter.hpp>

struct tolerance_tag : nova::parameter::parameter_tag< tolerance_tag >
{};

struct other_tag : nova::parameter::parameter_tag< other_tag >
{};

template < double D >
struct tolerance : nova::parameter::double_param< tolerance_tag, D >
{};

template < bool V >
struct flag : nova::parameter::bool_param< other_tag, V >
{};

// Should fail: tolerance_tag not in pack — static_assert fires
constexpr double val = nova::parameter::extract_double_v< tolerance_tag, flag< true > >;
