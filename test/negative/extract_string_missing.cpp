// SPDX-License-Identifier: MIT
// Negative test: extract_string_v with missing tag should fail to compile.

#include <nova/parameter/parameter.hpp>

struct name_tag : nova::parameter::parameter_tag< name_tag >
{};

struct other_tag : nova::parameter::parameter_tag< other_tag >
{};

template < nova::parameter::fixed_string S >
struct name : nova::parameter::string_param< name_tag, S >
{};

template < bool V >
struct flag : nova::parameter::bool_param< other_tag, V >
{};

// Should fail: name_tag not in pack — static_assert fires
constexpr auto val = nova::parameter::extract_string_v< name_tag, flag< true > >;
