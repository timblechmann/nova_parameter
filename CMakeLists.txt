cmake_minimum_required(VERSION 3.25)

project(nova_parameter VERSION 0.1.0 LANGUAGES CXX C)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
endif()

add_custom_target(nova_parameter_project_files SOURCES
    .clang-tidy
    .gitignore
    .pre-commit-config.yaml
    _clang-format
    Readme.md
    License.txt
    .github/workflows/ci.yml
)

# headers listing kept minimal for placeholder project
set(headers
    include/nova/parameter/parameter.hpp
)

set(sources
    source/nova/parameter/parameter.cpp
)

# Project layout
add_library(nova_parameter STATIC
    ${headers}
    ${sources}
)
add_library(nova::parameter ALIAS nova_parameter)

target_sources(nova_parameter PUBLIC FILE_SET HEADERS BASE_DIRS include)
target_sources(nova_parameter PUBLIC FILE_SET HEADERS FILES
    ${headers}
)

target_include_directories(nova_parameter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(CMakeDependentOption)

option(NOVA_PARAMETER_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
if(NOVA_PARAMETER_TESTS)
    # minimal test deps
    if (NOT TARGET Catch2::Catch2WithMain)
        if (NOT COMMAND CPMAddPackage)
            set(CPM_DOWNLOAD_VERSION 0.42.1)
            if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
            message(STATUS "Downloading CPM.cmake ${CPM_DOWNLOAD_VERSION}...")
            file(DOWNLOAD
                "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
                "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
            )
            endif()
            include("${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
        endif()
        CPMAddPackage(
            NAME Catch2
            GITHUB_REPOSITORY catchorg/Catch2
            VERSION 3.14.0
        )
    endif()

    include(CTest)
    include(Catch)

    add_executable(nova_parameter_tests
        test/test_main.cpp
    )
    target_link_libraries(nova_parameter_tests PRIVATE nova::parameter Catch2::Catch2WithMain)
    enable_testing()
    catch_discover_tests(nova_parameter_tests)

    # ---------------------------------------------------------------------------
    # Negative compile tests — expected NOT to compile under Clang with TSA.
    # Mirror upstream nova_sync negative test setup.
    # ---------------------------------------------------------------------------
    set(negative_compile_test
    )
    foreach(_neg_test IN LISTS negative_compile_test)
        add_executable(nova_parameter_neg_${_neg_test} EXCLUDE_FROM_ALL
            test/negative/${_neg_test}.cpp
        )
        target_link_libraries(nova_parameter_neg_${_neg_test} PRIVATE nova::parameter)
    endforeach()

    foreach(_neg_test IN LISTS negative_compile_test)
        add_test(
            NAME nova_parameter_neg_${_neg_test}
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                    --target nova_parameter_neg_${_neg_test}
                    --config $<CONFIGURATION>
        )
        set_tests_properties(nova_parameter_neg_${_neg_test} PROPERTIES
            WILL_FAIL TRUE
            LABELS "negative;thread-safety"
        )
    endforeach()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS nova_parameter
    EXPORT nova_parameterTargets
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT nova_parameterTargets
    FILE nova_parameterTargets.cmake
    NAMESPACE nova::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nova_parameter
)
