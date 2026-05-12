# nova::parameter

Header-only utilities for named, compile-time parameters for templates. The library provides
typed parameter helpers, compile-time extraction helpers, and validation utilities to enforce
allowed and required parameters.

## Key Features

- Type-safe parameter wrappers (type, bool, size, integral, float, double, flag, string).
- Non-type template parameters (NTTPs) as keyword arguments: integers, floating-point, and
  compile-time strings.
- Concept-style parameter with explanatory diagnostics (concept_param).
- Compile-time extraction helpers with three variants per type:
  - Required (compile error if absent): `extract_integral_v`, `extract_bool_v`, `extract_float_v`,
    `extract_double_v`, `extract_string_v`.
  - With compile-time default (silent fallback): `extract_integral_or_v`, `extract_bool_or_v`,
    `extract_float_or_v`, `extract_double_or_v`, `extract_string_or_v`.
  - Optional (std::optional, no sentinel): `extract_optional_integral_v`, `extract_optional_float_v`,
    `extract_optional_double_v`, `extract_optional_string_v`.
- Validation helpers and concepts: valid_parameters, required_parameters,
  validate_parameters, validate_required_parameters.

## Overview

**Architecture:** Parameters are type-safe wrappers around "tags" (unique marker types) and values.
A tag is a unique identifier (e.g., `capacity_tag`), and each parameter pairs a tag with a value
(e.g., `capacity< 128 >` wraps `capacity_tag` + `size_t(128)`). Callers pass multiple parameters as
template arguments forming a pack. Extraction utilities search the pack by tag and return the
associated value, with compile-time defaults if absent.

**Minimal Example:**

```c++
#include <nova/parameter/parameter.hpp>

// 1. Define a tag
struct size_tag : nova::parameter::parameter_tag< size_tag > {};

// 2. Create a parameter helper
template < std::size_t N >
struct size : nova::parameter::size_param< size_tag, N > {};

// 3. Define template with requirement
template < typename... Params >
    requires nova::parameter::required_parameters< std::tuple< size_tag >, Params... >
class buffer
{
public:
    static constexpr std::size_t capacity =
        nova::parameter::extract_integral_v< size_tag, std::size_t, Params... >;
};

// 4. Instantiate
using my_buffer = buffer< size< 256 > >;
static_assert( my_buffer::capacity == 256 );
```

## API (Summary)

```c++
namespace nova::parameter {

template < typename Derived >
struct parameter_tag; // inherit to define tag

template < typename Tag, typename Value >
struct parameter; // base wrapper

template < typename Tag, typename Value >
struct type_param; // type parameter
template < typename Tag, bool V >
struct bool_param; // bool parameter
template < typename Tag, size_t N >
struct size_param; // size_t parameter
template < typename Tag, typename T, T V >
struct integral_param; // any integral NTTP
template < typename Tag, float F >
struct float_param; // float NTTP
template < typename Tag, double D >
struct double_param; // double NTTP
template < typename Tag >
struct flag_param; // tag-only flag
template < typename Tag, fixed_string S >
struct string_param; // compile-time string parameter

// Compile-time string type — fixed-size, usable as NTTP
template < std::size_t N >
struct fixed_string { /* ... */ };

// Floating-point constant type for use as parameter value_type
template < typename FloatType, FloatType V >
struct float_constant { /* ... */ };

template < typename Tag, typename Default, typename... Params >
using extract_t = /* value type for Tag or Default */;

// Required extraction — compile error if Tag absent
template < typename Tag, typename IntegralType, typename... Params >
inline constexpr IntegralType extract_integral_v = /* integral value or compile error */;

template < typename Tag, typename... Params >
inline constexpr bool extract_bool_v = /* bool value or compile error */;

template < typename Tag, typename... Params >
inline constexpr float extract_float_v = /* float value or compile error */;

template < typename Tag, typename... Params >
inline constexpr double extract_double_v = /* double value or compile error */;

template < typename Tag, typename... Params >
inline constexpr auto extract_string_v = /* fixed_string value or compile error */;

// With compile-time default — silent fallback if Tag absent
template < typename Tag, typename IntegralType, IntegralType Default, typename... Params >
inline constexpr IntegralType extract_integral_or_v = /* integral value or Default */;

template < typename Tag, bool Default, typename... Params >
inline constexpr bool extract_bool_or_v = /* bool value or Default */;

template < typename Tag, float Default, typename... Params >
inline constexpr float extract_float_or_v = /* float value or Default */;

template < typename Tag, double Default, typename... Params >
inline constexpr double extract_double_or_v = /* double value or Default */;

template < typename Tag, fixed_string Default, typename... Params >
inline constexpr auto extract_string_or_v = /* fixed_string value or Default */;

// Optional extraction — std::nullopt when Tag is absent, no sentinel default needed
template < typename Tag, typename IntegralType, typename... Params >
inline constexpr std::optional< IntegralType > extract_optional_integral_v = /* value or nullopt */;

template < typename Tag, typename... Params >
inline constexpr std::optional< float > extract_optional_float_v = /* value or nullopt */;

template < typename Tag, typename... Params >
inline constexpr std::optional< double > extract_optional_double_v = /* value or nullopt */;

template < typename Tag, typename... Params >
inline constexpr std::optional< std::string_view > extract_optional_string_v = /* view or nullopt */;

template < typename AllowedTags, typename... Params >
concept valid_parameters = /* all parameters valid, no duplicates, allowed tags */;

template < typename RequiredTags, typename... Params >
concept required_parameters = /* all required tags present */;

} // namespace nova::parameter
```

## Small Working Example

```c++
#include <nova/parameter/parameter.hpp>

// define tags
struct allocator_tag : nova::parameter::parameter_tag< allocator_tag >
{};
struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};
struct fixed_tag : nova::parameter::parameter_tag< fixed_tag >
{};
struct name_tag : nova::parameter::parameter_tag< name_tag >
{};
struct threshold_tag : nova::parameter::parameter_tag< threshold_tag >
{};
struct tolerance_tag : nova::parameter::parameter_tag< tolerance_tag >
{};

// type and integral parameter helpers
template < typename A >
struct allocator : nova::parameter::type_param< allocator_tag, A >
{};
template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};
template < bool B >
struct fixed : nova::parameter::bool_param< fixed_tag, B >
{};

// string, float, and double NTTP parameter helpers
template < nova::parameter::fixed_string S >
struct name : nova::parameter::string_param< name_tag, S >
{};

template < float F >
struct threshold : nova::parameter::float_param< threshold_tag, F >
{};

template < double D >
struct tolerance : nova::parameter::double_param< tolerance_tag, D >
{};

using allowed  = std::tuple< allocator_tag, capacity_tag, fixed_tag, name_tag, threshold_tag, tolerance_tag >;
using required = std::tuple< capacity_tag >;

template < typename... Params >
    requires nova::parameter::valid_parameters< allowed, Params... >
             && nova::parameter::required_parameters< required, Params... >
class my_container
{
public:
    using allocator_type = nova::parameter::extract_t< allocator_tag, std::allocator< int >, Params... >;
    static constexpr std::size_t static_capacity =
        nova::parameter::extract_integral_v< capacity_tag, std::size_t, Params... >;
    static constexpr bool is_fixed =
        nova::parameter::extract_bool_or_v< fixed_tag, false, Params... >;
    static constexpr auto container_name =
        nova::parameter::extract_string_or_v< name_tag, "unnamed", Params... >;
    static constexpr float activation_threshold =
        nova::parameter::extract_float_or_v< threshold_tag, 1.0f, Params... >;
    static constexpr double precision_tolerance =
        nova::parameter::extract_double_or_v< tolerance_tag, 1e-6, Params... >;
};

// usage
using minimal = my_container< capacity< 128 > >; // required parameter provided

using full = my_container< allocator< std::allocator< double > >,
                           capacity< 256 >,
                           fixed< true >,
                           name< "my_buffer" >,  // compile-time string NTTP
                           threshold< 2.5f >,    // float NTTP
                           tolerance< 1e-9 > >; // double NTTP

// Extract values
constexpr std::string_view container_name = std::string_view( full::container_name );
constexpr float threshold = full::activation_threshold;
constexpr double eps      = full::precision_tolerance;
static_assert( container_name == "my_buffer" );
static_assert( threshold == 2.5f );
static_assert( eps == 1e-9 );

// optional extraction — std::nullopt when Tag absent, no sentinel default needed
constexpr auto cap    = nova::parameter::extract_optional_integral_v< capacity_tag, std::size_t, capacity< 128 > >;
constexpr auto no_cap = nova::parameter::extract_optional_integral_v< capacity_tag, std::size_t >;
static_assert( cap.has_value() && *cap == 128 );
static_assert( !no_cap.has_value() );

// optional float/double extraction
constexpr auto thr    = nova::parameter::extract_optional_float_v< threshold_tag, threshold< 1.5f > >;
constexpr auto no_thr = nova::parameter::extract_optional_float_v< threshold_tag >;
static_assert( thr.has_value() && *thr == 1.5f );
static_assert( !no_thr.has_value() );

constexpr auto tol    = nova::parameter::extract_optional_double_v< tolerance_tag, tolerance< 0.001 > >;
constexpr auto no_tol = nova::parameter::extract_optional_double_v< tolerance_tag >;
static_assert( tol.has_value() && *tol == 0.001 );
static_assert( !no_tol.has_value() );

// bool extraction with compile-time default
constexpr bool is_f = nova::parameter::extract_bool_or_v< fixed_tag, false, fixed< true > >;
static_assert( is_f );

// optional string extraction
constexpr auto found_name = nova::parameter::extract_optional_string_v< name_tag, name< "buffer" > >;
constexpr auto missing_name = nova::parameter::extract_optional_string_v< name_tag >;
static_assert( found_name.has_value() && *found_name == "buffer" );
static_assert( !missing_name.has_value() );
```

## Dependencies

- C++20 compiler.
- Catch2 for unit tests.

## Building

```
cmake -B build
cmake --build build
ctest --test-dir build
```

## References

This library is inspired by [boost.parameter](https://www.boost.org/doc/libs/latest/libs/parameter/doc/html/index.html). It tries to provide a similar functionality but with a cleaner and more modern API that takes advantage of C++20 features.

## License

MIT — see License.txt. Please use this code responsibly and ethically.
