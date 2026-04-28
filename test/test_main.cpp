// SPDX-License-Identifier: MIT

#include <nova/parameter/parameter.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Example parameter definitions (mimicking boost.lockfree style)

namespace example {

// Tags
struct allocator_tag : nova::parameter::parameter_tag< allocator_tag >
{};

struct fixed_sized_tag : nova::parameter::parameter_tag< fixed_sized_tag >
{};

struct capacity_tag : nova::parameter::parameter_tag< capacity_tag >
{};

struct mutex_tag : nova::parameter::parameter_tag< mutex_tag >
{};

struct lock_memory_tag : nova::parameter::parameter_tag< lock_memory_tag >
{};

// Parameters (using helpers)
template < typename Alloc >
struct allocator : nova::parameter::type_param< allocator_tag, Alloc >
{};

template < bool V >
struct fixed_sized : nova::parameter::bool_param< fixed_sized_tag, V >
{};

template < std::size_t N >
struct capacity : nova::parameter::size_param< capacity_tag, N >
{};

template < typename M = std::mutex >
struct use_mutex : nova::parameter::type_param< mutex_tag, M >
{};

struct lock_memory : nova::parameter::flag_param< lock_memory_tag >
{};

// Allowed tags tuple
using allowed_tags = std::tuple< allocator_tag, fixed_sized_tag, capacity_tag, mutex_tag, lock_memory_tag >;

// Required tags — capacity is required, everything else optional
using required_tags = std::tuple< capacity_tag >;

// Example data structure using parameters
template < typename... Params >
    requires nova::parameter::valid_parameters< allowed_tags, Params... >
             && nova::parameter::required_parameters< required_tags, Params... >
class my_container
{
    static constexpr auto validation = nova::parameter::validate_parameters< allowed_tags, Params... > {};
    static constexpr auto required_validation
        = nova::parameter::validate_required_parameters< required_tags, Params... > {};

public:
    using allocator_type = nova::parameter::extract_t< allocator_tag, std::allocator< int >, Params... >;

    static constexpr bool is_fixed_sized
        = nova::parameter::extract_integral_v< fixed_sized_tag, bool, false, Params... >;

    static constexpr std::size_t static_capacity
        = nova::parameter::extract_integral_v< capacity_tag, std::size_t, 0, Params... >;

    static constexpr bool has_mutex = nova::parameter::has_parameter_v< mutex_tag, Params... >;

    static constexpr bool has_lock_memory = nova::parameter::has_parameter_v< lock_memory_tag, Params... >;
};

// Optional-only container (no required params)
template < typename... Params >
    requires nova::parameter::valid_parameters< allowed_tags, Params... >
class optional_container
{
public:
    using allocator_type = nova::parameter::extract_t< allocator_tag, std::allocator< int >, Params... >;

    static constexpr std::size_t static_capacity
        = nova::parameter::extract_integral_v< capacity_tag, std::size_t, 0, Params... >;
};

// ---- concept_param example ----

// Traits struct: type must be an allocator (allocate/deallocate).
// Use std::bool_constant or any struct with ::value.
template < typename A >
using is_allocator_like = std::bool_constant< requires( A a, std::size_t n ) {
    { a.allocate( n ) } -> std::same_as< typename A::value_type* >;
    a.deallocate( nullptr, n );
} >;

struct allocator_concept_tag : nova::parameter::parameter_tag< allocator_concept_tag >
{};

// concept-constrained parameter: T must satisfy is_allocator_like
template < typename T >
struct typed_allocator : nova::parameter::concept_param< allocator_concept_tag, T, is_allocator_like >
{};

using concept_allowed_tags = std::tuple< allocator_concept_tag >;

template < typename... Params >
    requires nova::parameter::valid_parameters< concept_allowed_tags, Params... >
class concept_container
{
public:
    using allocator_type = nova::parameter::extract_t< allocator_concept_tag, std::allocator< int >, Params... >;
};

} // namespace example

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compile-time tests via static_assert

// is_parameter concept
static_assert( nova::parameter::is_parameter< example::allocator< std::allocator< int > > > );
static_assert( nova::parameter::is_parameter< example::fixed_sized< true > > );
static_assert( nova::parameter::is_parameter< example::capacity< 128 > > );
static_assert( nova::parameter::is_parameter< example::use_mutex<> > );
static_assert( nova::parameter::is_parameter< example::lock_memory > );
static_assert( !nova::parameter::is_parameter< int > );
static_assert( !nova::parameter::is_parameter< std::string > );

// has_parameter_v
static_assert( nova::parameter::has_parameter_v< example::capacity_tag, example::capacity< 64 > > );
static_assert( !nova::parameter::has_parameter_v< example::capacity_tag, example::fixed_sized< true > > );
static_assert( nova::parameter::has_parameter_v< example::mutex_tag,
                                                 example::fixed_sized< true >,
                                                 example::use_mutex<>,
                                                 example::capacity< 32 > > );

// count_parameter_v
static_assert( nova::parameter::count_parameter_v< example::capacity_tag, example::capacity< 64 > > == 1 );
static_assert(
    nova::parameter::count_parameter_v< example::capacity_tag, example::fixed_sized< true >, example::capacity< 64 > >
    == 1 );
static_assert( nova::parameter::count_parameter_v< example::capacity_tag, example::fixed_sized< true > > == 0 );

// all_parameters_v
static_assert( nova::parameter::all_parameters_v< example::capacity< 64 >, example::fixed_sized< true > > );
static_assert( nova::parameter::all_parameters_v<> );
static_assert( !nova::parameter::all_parameters_v< int, example::capacity< 64 > > );

// no_duplicate_tags_v
static_assert( nova::parameter::no_duplicate_tags_v< example::capacity< 64 >, example::fixed_sized< true > > );
static_assert( nova::parameter::no_duplicate_tags_v<> );
static_assert( !nova::parameter::no_duplicate_tags_v< example::capacity< 64 >, example::capacity< 128 > > );

// all_tags_allowed_v
static_assert(
    nova::parameter::all_tags_allowed_v< example::allowed_tags, example::capacity< 64 >, example::fixed_sized< true > > );
static_assert( nova::parameter::all_tags_allowed_v< example::allowed_tags > );

// extract_t
static_assert(
    std::is_same_v< nova::parameter::extract_t< example::allocator_tag, std::allocator< int >, example::capacity< 64 > >,
                    std::allocator< int > > ); // default used

static_assert( std::is_same_v< nova::parameter::extract_t< example::allocator_tag,
                                                           std::allocator< int >,
                                                           example::allocator< std::allocator< float > >,
                                                           example::capacity< 64 > >,
                               std::allocator< float > > ); // found

// extract_integral_v
static_assert( nova::parameter::extract_integral_v< example::capacity_tag, std::size_t, 0, example::capacity< 42 > >
               == 42 );
static_assert( nova::parameter::extract_integral_v< example::capacity_tag, std::size_t, 99, example::fixed_sized< true > >
               == 99 ); // default

// valid_parameters concept
static_assert(
    nova::parameter::valid_parameters< example::allowed_tags, example::capacity< 64 >, example::fixed_sized< true > > );
static_assert( nova::parameter::valid_parameters< example::allowed_tags > );

// required_parameters concept — capacity_tag required
static_assert( nova::parameter::required_parameters< example::required_tags, example::capacity< 64 > > );
static_assert(
    nova::parameter::required_parameters< example::required_tags, example::capacity< 64 >, example::fixed_sized< true > > );
static_assert( !nova::parameter::required_parameters< example::required_tags > );                    // missing capacity
static_assert(
    !nova::parameter::required_parameters< example::required_tags, example::fixed_sized< true > > ); // missing capacity

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// my_container compile-time checks (capacity is required)

// Fully configured
using full_container = example::my_container< example::allocator< std::allocator< double > >,
                                              example::fixed_sized< true >,
                                              example::capacity< 256 >,
                                              example::use_mutex< std::mutex >,
                                              example::lock_memory >;
static_assert( std::is_same_v< full_container::allocator_type, std::allocator< double > > );
static_assert( full_container::is_fixed_sized );
static_assert( full_container::static_capacity == 256 );
static_assert( full_container::has_mutex );
static_assert( full_container::has_lock_memory );

// Arbitrary order — same result
using reordered_container = example::my_container< example::lock_memory,
                                                   example::capacity< 256 >,
                                                   example::use_mutex< std::mutex >,
                                                   example::allocator< std::allocator< double > >,
                                                   example::fixed_sized< true > >;
static_assert( std::is_same_v< reordered_container::allocator_type, std::allocator< double > > );
static_assert( reordered_container::is_fixed_sized );
static_assert( reordered_container::static_capacity == 256 );
static_assert( reordered_container::has_mutex );
static_assert( reordered_container::has_lock_memory );

// Partial (only required capacity)
using minimal_container = example::my_container< example::capacity< 128 > >;
static_assert( std::is_same_v< minimal_container::allocator_type, std::allocator< int > > );
static_assert( !minimal_container::is_fixed_sized );
static_assert( minimal_container::static_capacity == 128 );
static_assert( !minimal_container::has_mutex );

// optional_container — no required params, zero-arg works
using default_optional = example::optional_container<>;
static_assert( default_optional::static_capacity == 0 );

// concept_param — valid allocator type works
using concept_cont = example::concept_container< example::typed_allocator< std::allocator< int > > >;
static_assert( std::is_same_v< concept_cont::allocator_type, std::allocator< int > > );

// concept_container with default (no param)
using concept_default = example::concept_container<>;
static_assert( std::is_same_v< concept_default::allocator_type, std::allocator< int > > );

// extract_bool_v
static_assert( nova::parameter::extract_bool_v< example::fixed_sized_tag, false, example::fixed_sized< true > > == true );
static_assert( nova::parameter::extract_bool_v< example::fixed_sized_tag, false, example::capacity< 64 > > == false );
static_assert( nova::parameter::extract_bool_v< example::fixed_sized_tag, true > == true ); // default

// extract_optional_integral_v
static_assert( nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t, example::capacity< 42 > >
               == 42 );
static_assert(
    !nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t, example::fixed_sized< true > >.has_value() );
static_assert( !nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t >.has_value() );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Runtime tests

TEST_CASE( "extract_t returns correct types at runtime", "[parameter]" )
{
    using alloc_t = nova::parameter::extract_t< example::allocator_tag,
                                                std::allocator< int >,
                                                example::allocator< std::allocator< char > >,
                                                example::capacity< 64 > >;
    alloc_t alloc;
    auto*   p = alloc.allocate( 1 );
    REQUIRE( p != nullptr );
    alloc.deallocate( p, 1 );
}

TEST_CASE( "my_container with required capacity only", "[parameter]" )
{
    example::my_container< example::capacity< 64 > > c;
    REQUIRE( !decltype( c )::is_fixed_sized );
    REQUIRE( decltype( c )::static_capacity == 64 );
    REQUIRE( !decltype( c )::has_mutex );
    REQUIRE( !decltype( c )::has_lock_memory );
}

TEST_CASE( "my_container with full parameters", "[parameter]" )
{
    example::my_container< example::capacity< 512 >, example::fixed_sized< true >, example::use_mutex<>, example::lock_memory >
        c;
    REQUIRE( decltype( c )::is_fixed_sized );
    REQUIRE( decltype( c )::static_capacity == 512 );
    REQUIRE( decltype( c )::has_mutex );
    REQUIRE( decltype( c )::has_lock_memory );
}

TEST_CASE( "my_container parameters in arbitrary order", "[parameter]" )
{
    example::my_container< example::lock_memory, example::capacity< 64 >, example::fixed_sized< true > > c1;
    example::my_container< example::fixed_sized< true >, example::lock_memory, example::capacity< 64 > > c2;
    example::my_container< example::capacity< 64 >, example::fixed_sized< true >, example::lock_memory > c3;

    REQUIRE( decltype( c1 )::static_capacity == 64 );
    REQUIRE( decltype( c2 )::static_capacity == 64 );
    REQUIRE( decltype( c3 )::static_capacity == 64 );

    REQUIRE( decltype( c1 )::is_fixed_sized );
    REQUIRE( decltype( c2 )::is_fixed_sized );
    REQUIRE( decltype( c3 )::is_fixed_sized );

    REQUIRE( decltype( c1 )::has_lock_memory );
    REQUIRE( decltype( c2 )::has_lock_memory );
    REQUIRE( decltype( c3 )::has_lock_memory );
}

TEST_CASE( "has_parameter_v runtime check", "[parameter]" )
{
    constexpr bool has_cap
        = nova::parameter::has_parameter_v< example::capacity_tag, example::fixed_sized< true >, example::capacity< 32 > >;
    REQUIRE( has_cap );

    constexpr bool no_cap = nova::parameter::has_parameter_v< example::capacity_tag, example::fixed_sized< true > >;
    REQUIRE( !no_cap );
}

TEST_CASE( "extract_integral_v runtime check", "[parameter]" )
{
    constexpr auto val
        = nova::parameter::extract_integral_v< example::capacity_tag, std::size_t, 0, example::capacity< 77 > >;
    REQUIRE( val == 77 );

    constexpr auto def
        = nova::parameter::extract_integral_v< example::capacity_tag, std::size_t, 42, example::fixed_sized< true > >;
    REQUIRE( def == 42 );
}

TEST_CASE( "optional_container with no parameters", "[parameter]" )
{
    example::optional_container<> c;
    REQUIRE( decltype( c )::static_capacity == 0 );
}

TEST_CASE( "optional_container with parameters", "[parameter]" )
{
    example::optional_container< example::capacity< 99 > > c;
    REQUIRE( decltype( c )::static_capacity == 99 );
}

TEST_CASE( "concept_param with valid allocator", "[parameter]" )
{
    example::concept_container< example::typed_allocator< std::allocator< int > > > c;
    using alloc = decltype( c )::allocator_type;
    alloc a;
    auto* p = a.allocate( 1 );
    REQUIRE( p != nullptr );
    a.deallocate( p, 1 );
}

TEST_CASE( "extract_bool_v", "[parameter]" )
{
    constexpr bool t = nova::parameter::extract_bool_v< example::fixed_sized_tag, false, example::fixed_sized< true > >;
    constexpr bool f = nova::parameter::extract_bool_v< example::fixed_sized_tag, false, example::capacity< 64 > >;
    REQUIRE( t );
    REQUIRE( !f );
}

TEST_CASE( "extract_optional_integral_v present", "[parameter]" )
{
    constexpr auto val
        = nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t, example::capacity< 77 > >;
    REQUIRE( val.has_value() );
    REQUIRE( *val == 77 );
}

TEST_CASE( "extract_optional_integral_v absent", "[parameter]" )
{
    constexpr auto val
        = nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t, example::fixed_sized< true > >;
    REQUIRE( !val.has_value() );
}

TEST_CASE( "extract_optional_integral_v empty pack", "[parameter]" )
{
    constexpr auto val = nova::parameter::extract_optional_integral_v< example::capacity_tag, std::size_t >;
    REQUIRE( !val.has_value() );
}

TEST_CASE( "concept_container with default allocator", "[parameter]" )
{
    example::concept_container<> c;
    using alloc = decltype( c )::allocator_type;
    REQUIRE( (std::is_same_v< alloc, std::allocator< int > >));
}
