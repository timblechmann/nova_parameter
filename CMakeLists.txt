cmake_minimum_required(VERSION 3.25)

project(nova_parameter VERSION 0.1.0 LANGUAGES CXX C)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL AND NOT CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
endif()

if (PROJECT_IS_TOP_LEVEL)
    add_custom_target(nova_parameter_project_files SOURCES
        .clang-tidy
        .gitignore
        .pre-commit-config.yaml
        _clang-format
        Readme.md
        License.txt
        .github/workflows/ci.yml
    )
endif()

set(headers
    include/nova/parameter/parameter.hpp
)

add_library(nova_parameter INTERFACE
    ${headers}
)
add_library(nova::parameter ALIAS nova_parameter)

target_sources(nova_parameter INTERFACE FILE_SET HEADERS BASE_DIRS include)
target_sources(nova_parameter INTERFACE FILE_SET HEADERS FILES
    ${headers}
)

target_include_directories(nova_parameter INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(CMakeDependentOption)

option(NOVA_PARAMETER_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
if(NOVA_PARAMETER_TESTS)
    if (NOT TARGET Catch2::Catch2WithMain)
        if (NOT COMMAND CPMAddPackage)
            set(CPM_DOWNLOAD_VERSION 0.42.1)
            if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
                message(STATUS "Downloading CPM.cmake ${CPM_DOWNLOAD_VERSION}...")
                file(DOWNLOAD
                    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
                    "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
                    STATUS _cpm_download_status
                    TLS_VERIFY ON
                )
                list(GET _cpm_download_status 0 _cpm_download_error)
                if(_cpm_download_error)
                    list(GET _cpm_download_status 1 _cpm_download_message)
                    message(FATAL_ERROR "Failed to download CPM.cmake: ${_cpm_download_message}")
                endif()
            endif()
            include("${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
        endif()
        CPMAddPackage(
            NAME Catch2
            GITHUB_REPOSITORY catchorg/Catch2
            VERSION 3.14.0
        )
        list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
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
    # Negative compile tests
    # ---------------------------------------------------------------------------
    set(negative_compile_test
        non_parameter_type
        duplicate_parameter
        unknown_parameter
        missing_required_parameter
        concept_param_violation
        extract_required_missing
    )
    foreach(_neg_test IN LISTS negative_compile_test)
        add_executable(nova_parameter_neg_${_neg_test} EXCLUDE_FROM_ALL
            test/negative/${_neg_test}.cpp
        )
        target_link_libraries(nova_parameter_neg_${_neg_test} PRIVATE nova::parameter)

        add_test(
            NAME nova_parameter_neg_${_neg_test}
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                    --target nova_parameter_neg_${_neg_test}
                    --config $<CONFIGURATION>
        )
        set_tests_properties(nova_parameter_neg_${_neg_test} PROPERTIES
            WILL_FAIL TRUE
            LABELS "negative"
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
