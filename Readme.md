# nova::parameter

Header-only utilities for named, compile-time parameters for templates. The
library provides typed parameter helpers, compile-time extraction helpers, and
validation utilities to enforce allowed and required parameters.

Key features
* Type-safe parameter wrappers (type, bool, size, integral, flag).
* Concept-style parameter with explanatory diagnostics (concept_param).
* Compile-time extraction helpers: extract_t, extract_integral_v, extract_bool_v.
* Optional extraction without sentinel defaults: extract_optional_integral_v returns std::optional<T>.
* Validation helpers and concepts: valid_parameters, required_parameters,
  validate_parameters, validate_required_parameters.

Example API (summary)
```c++
namespace nova::parameter {

template<typename Derived> struct parameter_tag; // inherit to define tag

template<typename Tag, typename Value> struct parameter; // base wrapper

template<typename Tag, typename Value> struct type_param;  // type parameter
template<typename Tag, bool V>      struct bool_param;  // bool parameter
template<typename Tag, size_t N>    struct size_param;  // size_t parameter
template<typename Tag>              struct flag_param;  // tag-only flag

template<typename Tag, typename Default, typename... Params>
using extract_t = /* value type for Tag or Default */;

template<typename Tag, typename IntegralType, IntegralType Default, typename... Params>
inline constexpr IntegralType extract_integral_v = /* integral value or Default */;

template<typename Tag, bool Default, typename... Params>
inline constexpr bool extract_bool_v = /* bool value or Default */;

// Optional extraction — std::nullopt when Tag is absent, no sentinel default needed
template<typename Tag, typename IntegralType, typename... Params>
inline constexpr std::optional<IntegralType> extract_optional_integral_v = /* value or nullopt */;

template<typename AllowedTags, typename... Params>
concept valid_parameters = /* all parameters valid, no duplicates, allowed tags */;

template<typename RequiredTags, typename... Params>
concept required_parameters = /* all required tags present */;

} // namespace nova::parameter
```

Small working example
```c++
#include <nova/parameter/parameter.hpp>

// define tags
struct allocator_tag : nova::parameter::parameter_tag<allocator_tag> {};
struct capacity_tag  : nova::parameter::parameter_tag<capacity_tag> {};
struct fixed_tag     : nova::parameter::parameter_tag<fixed_tag> {};

// parameter helpers
template<typename A> struct allocator  : nova::parameter::type_param<allocator_tag, A> {};
template<std::size_t N> struct capacity : nova::parameter::size_param<capacity_tag, N> {};
template<bool B> struct fixed      : nova::parameter::bool_param<fixed_tag, B> {};

using allowed  = std::tuple<allocator_tag, capacity_tag, fixed_tag>;
using required = std::tuple<capacity_tag>;

template<typename... Params>
    requires nova::parameter::valid_parameters<allowed, Params...>
          && nova::parameter::required_parameters<required, Params...>
class my_container {
public:
    using allocator_type = nova::parameter::extract_t<allocator_tag, std::allocator<int>, Params...>;
    static constexpr std::size_t static_capacity =
        nova::parameter::extract_integral_v<capacity_tag, std::size_t, 0, Params...>;
    static constexpr bool is_fixed = nova::parameter::extract_integral_v<fixed_tag, bool, false, Params...>;
};

// usage
using minimal = my_container<capacity<128>>; // required parameter provided
using full = my_container<allocator<std::allocator<double>>, capacity<256>, fixed<true>>;

// optional extraction — std::nullopt when Tag absent, no sentinel default needed
constexpr auto cap    = nova::parameter::extract_optional_integral_v<capacity_tag, std::size_t, capacity<128>>;
constexpr auto no_cap = nova::parameter::extract_optional_integral_v<capacity_tag, std::size_t>;
static_assert( cap.has_value() && *cap == 128);
static_assert(!no_cap.has_value());

// bool extraction with compile-time default
constexpr bool is_f = nova::parameter::extract_bool_v<fixed_tag, /*default=*/false, fixed<true>>;
static_assert(is_f);
```

Dependencies
* C++20 compiler.
* Catch2 for unit tests.

Building
```
cmake -B build
cmake --build build
ctest --test-dir build
```

License
MIT — see License.txt. Please use this code responsibly and ethically.
