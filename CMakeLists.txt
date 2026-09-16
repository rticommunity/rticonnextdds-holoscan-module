# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.30)

project(rticonnextdds_holoscan_module
    VERSION 0.1.0
    DESCRIPTION "RTI Connext DDS integration for NVIDIA Holoscan"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(holoscan 5.0.0 REQUIRED CONFIG)

if(NOT DEFINED ENV{NDDSHOME} OR "$ENV{NDDSHOME}" STREQUAL "")
    message(FATAL_ERROR "NDDSHOME must point to an RTI Connext DDS installation")
endif()

list(PREPEND CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
find_package(RTIConnextDDS 7.7.0 REQUIRED)

include(CTest)
include(cmake/RTIConnextDDSGenerateIdl.cmake)

add_library(rti_connext_dds_operators INTERFACE)
add_library(RTIConnextDDSHoloscan::dds ALIAS rti_connext_dds_operators)
target_compile_features(rti_connext_dds_operators INTERFACE cxx_std_20)
target_include_directories(rti_connext_dds_operators
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(rti_connext_dds_operators
    INTERFACE
        holoscan::core
        RTIConnextDDS::cpp2_api
        Threads::Threads
)

rti_holoscan_add_idl(
    TARGET example_telemetry_types
    IDL examples/two_idl/idl/Telemetry.idl
)
rti_holoscan_add_idl(
    TARGET example_command_types
    IDL examples/two_idl/idl/Command.idl
)

set(two_idl_example_dir "${CMAKE_CURRENT_BINARY_DIR}/examples/two_idl")
file(MAKE_DIRECTORY "${two_idl_example_dir}")
# Remove the filename used by an earlier prototype. Connext auto-loads this
# special name and would otherwise load the same profile twice after an
# incremental reconfigure.
file(REMOVE "${two_idl_example_dir}/USER_QOS_PROFILES.xml")
configure_file(
    examples/two_idl/HoloscanConnextQos.xml
    "${two_idl_example_dir}/HoloscanConnextQos.xml"
    COPYONLY
)

add_executable(connext_two_idl_publisher
    examples/two_idl/publisher.cpp
)
target_include_directories(connext_two_idl_publisher
    PRIVATE examples/two_idl
)
target_link_libraries(connext_two_idl_publisher
    PRIVATE
        RTIConnextDDSHoloscan::dds
        example_telemetry_types
        example_command_types
)

add_executable(connext_two_idl_subscriber
    examples/two_idl/subscriber.cpp
)
target_include_directories(connext_two_idl_subscriber
    PRIVATE examples/two_idl
)
target_link_libraries(connext_two_idl_subscriber
    PRIVATE
        RTIConnextDDSHoloscan::dds
        example_telemetry_types
        example_command_types
)

add_executable(holoscan_connext_baseline_smoke
    tests/baseline_smoke.cpp
)
target_link_libraries(holoscan_connext_baseline_smoke
    PRIVATE
        holoscan::core
        RTIConnextDDS::cpp2_api
        Threads::Threads
)

if(BUILD_TESTING)
    add_test(
        NAME holoscan_connext_baseline_smoke
        COMMAND holoscan_connext_baseline_smoke
    )
    set_tests_properties(holoscan_connext_baseline_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "Holoscan 5 and RTI Connext DDS baseline passed"
        TIMEOUT 30
    )

    add_executable(connext_generated_types_smoke
        tests/generated_types_smoke.cpp
    )
    target_link_libraries(connext_generated_types_smoke
        PRIVATE
            example_telemetry_types
            example_command_types
    )
    add_test(
        NAME connext_generated_types_smoke
        COMMAND connext_generated_types_smoke
    )
    set_tests_properties(connext_generated_types_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "Two application-owned IDL types generated and usable"
        TIMEOUT 30
    )

    add_test(
        NAME connext_two_idl_integration
        COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_two_idl_integration.sh"
            "$<TARGET_FILE:connext_two_idl_subscriber>"
            "$<TARGET_FILE:connext_two_idl_publisher>"
    )
    set_tests_properties(connext_two_idl_integration PROPERTIES
        WORKING_DIRECTORY "${two_idl_example_dir}"
        PASS_REGULAR_EXPRESSION
            "DDS subscriber received and validated Telemetry=20, Command=20 samples"
        TIMEOUT 45
    )
endif()
