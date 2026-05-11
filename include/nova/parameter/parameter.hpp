// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Tim Blechmann

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace nova::parameter {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parameter_tag: base class for parameter tags. Each tag identifies a "slot" in the parameter pack.

template < typename Derived >
struct parameter_tag
{};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parameter: wraps a tag + value type. Users inherit from this or use the helpers below.

template < typename Tag, typename Value >
struct parameter
{
    using tag_type   = Tag;
    using value_type = Value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// concept: is_parameter — checks if T is a parameter (has tag_type and value_type)

template < typename T >
concept is_parameter = requires {
    typename T::tag_type;
    typename T::value_type;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// detail

namespace detail {

// ---- extract: find first parameter matching Tag, or return Default ----

template < typename Tag, typename Default, typename... Params >
struct extract_impl
{
    using type = Default;
};

template < typename Tag, typename Default, typename Head, typename... Tail >
    requires is_parameter< Head >
struct extract_impl< Tag, Default, Head, Tail... >
{
    using type = std::conditional_t< std::is_same_v< typename Head::tag_type, Tag >,
                                     typename Head::value_type,
                                     typename extract_impl< Tag, Default, Tail... >::type >;
};

// ---- has_parameter: check if Tag exists in pack ----

template < typename Tag, typename... Params >
struct has_parameter_impl : std::false_type
{};

template < typename Tag, typename Head, typename... Tail >
    requires is_parameter< Head >
struct has_parameter_impl< Tag, Head, Tail... >
{
    static constexpr bool value = std::is_same_v< typename Head::tag_type, Tag >
                                  || has_parameter_impl< Tag, Tail... >::value;
};

// ---- count_parameter: count occurrences of Tag in pack ----

template < typename Tag, typename... Params >
struct count_parameter_impl
{
    static constexpr int value = 0;
};

template < typename Tag, typename Head, typename... Tail >
    requires is_parameter< Head >
struct count_parameter_impl< Tag, Head, Tail... >
{
    static constexpr int value
        = ( std::is_same_v< typename Head::tag_type, Tag > ? 1 : 0 ) + count_parameter_impl< Tag, Tail... >::value;
};

// ---- all_parameters: check all types in pack satisfy is_parameter ----

template < typename... Ts >
struct all_parameters_impl : std::true_type
{};

template < typename Head, typename... Tail >
struct all_parameters_impl< Head, Tail... >
{
    static constexpr bool value = is_parameter< Head > && all_parameters_impl< Tail... >::value;
};

// ---- is_one_of_tags: check if a tag is one of the allowed tags ----

template < typename Tag, typename... AllowedTags >
struct is_one_of_tags_impl : std::false_type
{};

template < typename Tag, typename First, typename... Rest >
struct is_one_of_tags_impl< Tag, First, Rest... >
{
    static constexpr bool value = std::is_same_v< Tag, First > || is_one_of_tags_impl< Tag, Rest... >::value;
};

// ---- all_tags_allowed: check all parameters use allowed tags ----

template < typename AllowedTagsTuple, typename... Params >
struct all_tags_allowed_impl;

template < typename... AllowedTags >
struct all_tags_allowed_impl< std::tuple< AllowedTags... > >
{
    static constexpr bool value = true;
};

template < typename... AllowedTags, typename Head, typename... Tail >
struct all_tags_allowed_impl< std::tuple< AllowedTags... >, Head, Tail... >
{
    static constexpr bool value = is_one_of_tags_impl< typename Head::tag_type, AllowedTags... >::value
                                  && all_tags_allowed_impl< std::tuple< AllowedTags... >, Tail... >::value;
};

// ---- no_duplicate_tags: ensure each tag appears at most once ----

template < typename... Params >
struct no_duplicate_tags_impl;

template <>
struct no_duplicate_tags_impl<>
{
    static constexpr bool value = true;
};

template < typename Head, typename... Tail >
struct no_duplicate_tags_impl< Head, Tail... >
{
    static constexpr bool value = ( count_parameter_impl< typename Head::tag_type, Tail... >::value == 0 )
                                  && no_duplicate_tags_impl< Tail... >::value;
};

// ---- sentinel for "not found" ----

struct not_found_t
{};

// ---- check_required: verify all required tags are present (pure bool, no side effects) ----

template < typename RequiredTagsTuple, typename... Params >
struct check_required_impl;

template < typename... Params >
struct check_required_impl< std::tuple<>, Params... >
{
    static constexpr bool value = true;
};

template < typename HeadTag, typename... TailTags, typename... Params >
struct check_required_impl< std::tuple< HeadTag, TailTags... >, Params... >
{
    using found                      = extract_impl< HeadTag, not_found_t, Params... >;
    static constexpr bool this_found = !std::is_same_v< typename found::type, not_found_t >;
    static constexpr bool value      = this_found && check_required_impl< std::tuple< TailTags... >, Params... >::value;
};

// ---- assert_required: same logic but fires static_assert with diagnostic ----

template < typename RequiredTagsTuple, typename... Params >
struct assert_required_impl;

template < typename... Params >
struct assert_required_impl< std::tuple<>, Params... >
{
    static constexpr bool value = true;
};

template < typename HeadTag, typename... TailTags, typename... Params >
struct assert_required_impl< std::tuple< HeadTag, TailTags... >, Params... >
{
    using found                      = extract_impl< HeadTag, not_found_t, Params... >;
    static constexpr bool this_found = !std::is_same_v< typename found::type, not_found_t >;

    static_assert( this_found,
                   "Required parameter is missing. See 'HeadTag' in the enclosing template instantiation for the "
                   "missing tag." );

    static constexpr bool value = this_found && assert_required_impl< std::tuple< TailTags... >, Params... >::value;
};

// ---- extract_required: like extract_impl but static_asserts if tag not found ----

template < typename Tag, typename... Params >
struct extract_required_impl
{
    using found = extract_impl< Tag, not_found_t, Params... >;
    static_assert( !std::is_same_v< typename found::type, not_found_t >,
                   "extract_required: required tag is missing from parameter list. "
                   "See 'Tag' in the enclosing template instantiation." );
    using type = typename found::type;
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// fixed_string: compile-time string type usable as NTTP (C++20)

/// Compile-time fixed-length string. Use as NTTP in string_param.
/// Example: fixed_string{"hello"} — length includes null terminator.
template < std::size_t N >
struct fixed_string
{
    char data[ N ] {};

    constexpr fixed_string( const char ( &s )[ N ] )
    {
        std::copy_n( s, N, data );
    }

    constexpr auto operator<=>( const fixed_string& ) const = default;
    constexpr auto operator<=>( std::string_view rhs ) const
    {
        return std::string_view { *this } <=> rhs;
    }
    constexpr explicit operator std::string_view() const
    {
        return { data, N - 1 }; // exclude null terminator
    }
};

template < std::size_t N >
fixed_string( const char ( & )[ N ] ) -> fixed_string< N >;

/// Holds a compile-time string as a type-level constant (analogous to std::integral_constant).
template < fixed_string S >
struct string_constant
{
    static constexpr auto value = S;
};

/// Holds a compile-time floating-point value as a type-level constant (analogous to std::integral_constant).
/// C++20 allows floating-point NTTPs; this type wraps them for use as parameter value_type.
template < typename FloatType, FloatType V >
struct float_constant
{
    static constexpr FloatType value = V;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// public API — extraction

/// Extract value type for Tag from Params, or Default if absent.
template < typename Tag, typename Default, typename... Params >
using extract_t = typename detail::extract_impl< Tag, Default, Params... >::type;

/// Check if Tag present in Params.
template < typename Tag, typename... Params >
inline constexpr bool has_parameter_v = detail::has_parameter_impl< Tag, Params... >::value;

/// Count occurrences of Tag in Params.
template < typename Tag, typename... Params >
inline constexpr int count_parameter_v = detail::count_parameter_impl< Tag, Params... >::value;

/// True if all types in pack are valid parameters.
template < typename... Params >
inline constexpr bool all_parameters_v = detail::all_parameters_impl< Params... >::value;

/// True if no tag appears more than once.
template < typename... Params >
inline constexpr bool no_duplicate_tags_v = detail::no_duplicate_tags_impl< Params... >::value;

/// True if all parameter tags are in AllowedTags tuple.
template < typename AllowedTagsTuple, typename... Params >
inline constexpr bool all_tags_allowed_v = detail::all_tags_allowed_impl< AllowedTagsTuple, Params... >::value;

/// Extract value type for Tag from Params — static_assert if Tag absent.
template < typename Tag, typename... Params >
using extract_required_t = typename detail::extract_required_impl< Tag, Params... >::type;

/// Extract integral value — static_assert if Tag absent.
template < typename Tag, typename IntegralType, typename... Params >
inline constexpr IntegralType extract_integral_v = extract_required_t< Tag, Params... >::value;

/// Extract integral value with compile-time default (returned when Tag is absent).
template < typename Tag, typename IntegralType, IntegralType Default, typename... Params >
inline constexpr IntegralType extract_integral_or_v
    = extract_t< Tag, std::integral_constant< IntegralType, Default >, Params... >::value;

/// Extract bool — static_assert if Tag absent.
template < typename Tag, typename... Params >
inline constexpr bool extract_bool_v = extract_integral_v< Tag, bool, Params... >;

/// Extract bool with compile-time default.
template < typename Tag, bool Default, typename... Params >
inline constexpr bool extract_bool_or_v = extract_integral_or_v< Tag, bool, Default, Params... >;

/// Extract integral value as std::optional<IntegralType>.
/// Contains a value if Tag is present in Params, std::nullopt otherwise.
template < typename Tag, typename IntegralType, typename... Params >
inline constexpr std::optional< IntegralType > extract_optional_integral_v = []() constexpr {
    using sentinel   = detail::not_found_t;
    using found_type = extract_t< Tag, sentinel, Params... >;
    if constexpr ( std::is_same_v< found_type, sentinel > )
        return std::optional< IntegralType > {};
    else
        return std::optional< IntegralType > { static_cast< IntegralType >( found_type::value ) };
}();

/// Extract fixed_string — static_assert if Tag absent.
template < typename Tag, typename... Params >
inline constexpr auto extract_string_v = extract_required_t< Tag, Params... >::value;

/// Extract fixed_string with compile-time default.
template < typename Tag, fixed_string Default, typename... Params >
inline constexpr auto extract_string_or_v = extract_t< Tag, string_constant< Default >, Params... >::value;

/// Extract fixed_string as std::optional<std::string_view>.
/// Contains a value if Tag is present, std::nullopt otherwise.
template < typename Tag, typename... Params >
inline constexpr std::optional< std::string_view > extract_optional_string_v = []() constexpr {
    using sentinel   = detail::not_found_t;
    using found_type = extract_t< Tag, sentinel, Params... >;
    if constexpr ( std::is_same_v< found_type, sentinel > )
        return std::optional< std::string_view > {};
    else
        return std::optional< std::string_view > { found_type::value };
}();

/// Extract float — static_assert if Tag absent.
template < typename Tag, typename... Params >
inline constexpr float extract_float_v = extract_required_t< Tag, Params... >::value;

/// Extract float with compile-time default.
template < typename Tag, float Default, typename... Params >
inline constexpr float extract_float_or_v = extract_t< Tag, float_constant< float, Default >, Params... >::value;

/// Extract float as std::optional<float>.
/// Contains a value if Tag is present, std::nullopt otherwise.
template < typename Tag, typename... Params >
inline constexpr std::optional< float > extract_optional_float_v = []() constexpr {
    using sentinel   = detail::not_found_t;
    using found_type = extract_t< Tag, sentinel, Params... >;
    if constexpr ( std::is_same_v< found_type, sentinel > )
        return std::optional< float > {};
    else
        return std::optional< float > { found_type::value };
}();

/// Extract double — static_assert if Tag absent.
template < typename Tag, typename... Params >
inline constexpr double extract_double_v = extract_required_t< Tag, Params... >::value;

/// Extract double with compile-time default.
template < typename Tag, double Default, typename... Params >
inline constexpr double extract_double_or_v = extract_t< Tag, float_constant< double, Default >, Params... >::value;

/// Extract double as std::optional<double>.
/// Contains a value if Tag is present, std::nullopt otherwise.
template < typename Tag, typename... Params >
inline constexpr std::optional< double > extract_optional_double_v = []() constexpr {
    using sentinel   = detail::not_found_t;
    using found_type = extract_t< Tag, sentinel, Params... >;
    if constexpr ( std::is_same_v< found_type, sentinel > )
        return std::optional< double > {};
    else
        return std::optional< double > { found_type::value };
}();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// public API — validation

/// Concept: pack forms valid parameters — all are parameters, no duplicates, all tags allowed.
template < typename AllowedTags, typename... Params >
concept valid_parameters
    = all_parameters_v< Params... > && no_duplicate_tags_v< Params... > && all_tags_allowed_v< AllowedTags, Params... >;

/// Concept: all required tags (RequiredTagsTuple) are present in Params.
/// Triggers a static_assert with the missing tag type when a required parameter is absent.
template < typename RequiredTagsTuple, typename... Params >
concept required_parameters = detail::check_required_impl< RequiredTagsTuple, Params... >::value;

/// validate_parameters: struct that fires static_assert with clear messages.
/// AllowedTags = std::tuple<Tag1, Tag2, ...>
template < typename AllowedTags, typename... Params >
struct validate_parameters
{
    static_assert( all_parameters_v< Params... >,
                   "All template arguments must be parameters (have tag_type/value_type)" );
    static_assert( no_duplicate_tags_v< Params... >, "Duplicate parameter tags are not allowed" );
    static_assert( all_tags_allowed_v< AllowedTags, Params... >, "Unknown parameter tag provided" );

    static constexpr bool valid = all_parameters_v< Params... > && no_duplicate_tags_v< Params... >
                                  && all_tags_allowed_v< AllowedTags, Params... >;
};

/// validate_required_parameters: fires static_assert naming the missing tag when a required param is absent.
/// RequiredTags = std::tuple<Tag1, Tag2, ...>
/// Intended for use inside a class body (not as a concept) to get richer diagnostics.
template < typename RequiredTags, typename... Params >
struct validate_required_parameters
{
    static constexpr bool valid = detail::assert_required_impl< RequiredTags, Params... >::value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// parameter helpers: make defining parameters easy

/// Type parameter: wraps a type under a tag.
/// Usage: `template <typename T> struct my_alloc : type_param<my_alloc_tag, T> {};`
template < typename Tag, typename Value >
struct type_param : parameter< Tag, Value >
{};

/// Bool parameter: wraps a bool under a tag.
/// Usage: `template <bool V> struct fixed : bool_param<fixed_tag, V> {};`
template < typename Tag, bool V >
struct bool_param : parameter< Tag, std::bool_constant< V > >
{};

/// Size parameter: wraps a size_t under a tag.
/// Usage: `template <size_t N> struct capacity : size_param<capacity_tag, N> {};`
template < typename Tag, std::size_t N >
struct size_param : parameter< Tag, std::integral_constant< std::size_t, N > >
{};

/// Integral parameter: wraps any integral value under a tag.
template < typename Tag, typename IntegralType, IntegralType V >
struct integral_param : parameter< Tag, std::integral_constant< IntegralType, V > >
{};

/// Float parameter: wraps a float under a tag.
/// Usage: `template <float F> struct threshold : float_param<threshold_tag, F> {};`
template < typename Tag, float F >
struct float_param : parameter< Tag, float_constant< float, F > >
{};

/// Double parameter: wraps a double under a tag.
/// Usage: `template <double D> struct epsilon : double_param<epsilon_tag, D> {};`
template < typename Tag, double D >
struct double_param : parameter< Tag, float_constant< double, D > >
{};

/// Flag parameter: tag-only, no meaningful value. value_type = std::true_type.
/// Usage: `struct enable_foo : flag_param<enable_foo_tag> {};`
template < typename Tag >
struct flag_param : parameter< Tag, std::true_type >
{};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// string_param: wraps a fixed_string NTTP under a tag

/// String parameter: wraps a compile-time string literal under a tag.
/// Usage:
///   template <nova::parameter::fixed_string S>
///   struct name : nova::parameter::string_param<name_tag, S> {};
template < typename Tag, fixed_string S >
struct string_param : parameter< Tag, string_constant< S > >
{};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// concept_param: type parameter whose value must satisfy a concept.
//
// Usage:
//   template <typename T>
//   struct allocator : concept_param<allocator_tag, T, std::allocator_of<int>> {};
//
// The concept is checked at definition of the parameter, giving a clear error
// at the call site naming both the tag and the concept.

/// concept_param: type parameter whose value must satisfy a compile-time traits check.
///
/// Constraint is a traits struct template with a static bool `::value`.
/// Use std::bool_constant, std::is_integral, std::is_trivial, etc., or define your own.
///
/// Usage:
///   // Traits struct:
///   template <typename T>
///   using must_be_integral = std::bool_constant<std::is_integral_v<T>>;
///
///   // Parameter definition:
///   template <typename T>
///   struct my_int_param : nova::parameter::concept_param<my_tag, T, must_be_integral> {};
///
/// Triggers a static_assert with a descriptive message when Value fails Constraint.

template < typename Tag, typename Value, template < typename > typename Constraint >
struct concept_param : parameter< Tag, Value >
{
    static_assert( Constraint< Value >::value,
                   "concept_param: Value does not satisfy the required constraint. "
                   "Check the Tag and Constraint template arguments." );
};

} // namespace nova::parameter
