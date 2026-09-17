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

# Use the ShapeTypeExtended definition shipped with Connext so the example is
# wire-compatible with RTI Shapes Demo without keeping a second copy of RTI's
# IDL in this repository.
rti_holoscan_add_idl(
    TARGET shapes_demo_dds_types
    IDL "$ENV{NDDSHOME}/resource/idl/ShapeType.idl"
)

set(shapes_demo_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/shapes_demo_payload")
holoscan_add_flatbuffer_schema(
    TARGET shapes_demo_payload_generated
    SCHEMAS examples/shapes_demo/shape.fbs
    INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/examples/shapes_demo"
    OUTPUT_DIR "${shapes_demo_generated_dir}"
    BFBS_EMBED
    TRAITS_HEADER shape_schema_traits.hpp
    TRAITS_SCHEMA examples/shapes_demo/shape.fbs
    TRAITS_ROOT_TYPE rti::holoscan::shapes::Shape
    TRAITS_NATIVE_TYPE rti::holoscan::shapes::ShapeT
    TRAITS_BINARY_SCHEMA_TYPE rti::holoscan::shapes::ShapeBinarySchema
    # Keep the generated evidence namespace outside rti::holoscan. The EA2
    # generator emits unqualified holoscan:: references inside that namespace.
    TRAITS_PACKAGE_NAMESPACE rti_holoscan_shapes::package
    TRAITS_CANONICAL_NAME rti.holoscan.shapes.Shape
    TRAITS_SCHEMA_VERSION 1
    TRAITS_MAX_SERIALIZED_SIZE 512
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

set(shapes_demo_example_dir "${CMAKE_CURRENT_BINARY_DIR}/examples/shapes_demo")
file(MAKE_DIRECTORY "${shapes_demo_example_dir}")
configure_file(
    examples/shapes_demo/HoloscanConnextQos.xml
    "${shapes_demo_example_dir}/HoloscanConnextQos.xml"
    COPYONLY
)

add_executable(connext_shapes_publisher
    examples/shapes_demo/publisher.cpp
)
add_dependencies(connext_shapes_publisher shapes_demo_payload_generated)
target_include_directories(connext_shapes_publisher
    PRIVATE
        examples/shapes_demo
        "${shapes_demo_generated_dir}"
)
target_link_libraries(connext_shapes_publisher
    PRIVATE
        RTIConnextDDSHoloscan::dds
        holoscan::schema
        shapes_demo_dds_types
)

add_executable(connext_shapes_subscriber
    examples/shapes_demo/subscriber.cpp
)
add_dependencies(connext_shapes_subscriber shapes_demo_payload_generated)
target_include_directories(connext_shapes_subscriber
    PRIVATE
        examples/shapes_demo
        "${shapes_demo_generated_dir}"
)
target_link_libraries(connext_shapes_subscriber
    PRIVATE
        RTIConnextDDSHoloscan::dds
        holoscan::schema
        shapes_demo_dds_types
)

set(shapes_xcdr_example_dir "${CMAKE_CURRENT_BINARY_DIR}/examples/shapes_xcdr")
file(MAKE_DIRECTORY "${shapes_xcdr_example_dir}")
configure_file(
    examples/shapes_demo/HoloscanConnextQos.xml
    "${shapes_xcdr_example_dir}/HoloscanConnextQos.xml"
    COPYONLY
)

foreach(xcdr_role IN ITEMS publisher subscriber)
    add_executable(connext_shapes_xcdr_${xcdr_role}
        examples/shapes_xcdr/${xcdr_role}.cpp
    )
    add_dependencies(connext_shapes_xcdr_${xcdr_role} shapes_demo_payload_generated)
    target_include_directories(connext_shapes_xcdr_${xcdr_role}
        PRIVATE
            examples/shapes_demo
            examples/shapes_xcdr
            "${shapes_demo_generated_dir}"
    )
    target_link_libraries(connext_shapes_xcdr_${xcdr_role}
        PRIVATE
            RTIConnextDDSHoloscan::dds
            holoscan::schema
            shapes_demo_dds_types
    )
endforeach()

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

    add_executable(connext_shapes_adapter_test
        tests/shapes_adapter_test.cpp
    )
    add_dependencies(connext_shapes_adapter_test shapes_demo_payload_generated)
    target_include_directories(connext_shapes_adapter_test
        PRIVATE
            examples/shapes_demo
            "${shapes_demo_generated_dir}"
    )
    target_link_libraries(connext_shapes_adapter_test
        PRIVATE
            holoscan::core
            holoscan::schema
            shapes_demo_dds_types
    )
    add_test(
        NAME connext_shapes_adapter_test
        COMMAND connext_shapes_adapter_test
    )
    set_tests_properties(connext_shapes_adapter_test PROPERTIES
        PASS_REGULAR_EXPRESSION "Shapes adapter preserved all ShapeTypeExtended fields"
        TIMEOUT 30
    )

    add_test(
        NAME connext_shapes_integration
        COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_shapes_integration.sh"
            "$<TARGET_FILE:connext_shapes_subscriber>"
            "$<TARGET_FILE:connext_shapes_publisher>"
    )
    set_tests_properties(connext_shapes_integration PROPERTIES
        WORKING_DIRECTORY "${shapes_demo_example_dir}"
        PASS_REGULAR_EXPRESSION "DDS Shapes subscriber received 20 ShapeTypeExtended samples"
        TIMEOUT 45
    )

    add_executable(connext_shapes_conversion_benchmark
        tests/shapes_conversion_benchmark.cpp
    )
    add_dependencies(connext_shapes_conversion_benchmark shapes_demo_payload_generated)
    target_include_directories(connext_shapes_conversion_benchmark
        PRIVATE
            examples/shapes_demo
            examples/shapes_xcdr
            "${shapes_demo_generated_dir}"
    )
    target_link_libraries(connext_shapes_conversion_benchmark
        PRIVATE
            holoscan::core
            holoscan::schema
            shapes_demo_dds_types
    )
    add_test(
        NAME connext_shapes_conversion_benchmark_smoke
        COMMAND connext_shapes_conversion_benchmark --iterations 1000
    )
    set_tests_properties(connext_shapes_conversion_benchmark_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "xcdr_overhead_ratio="
        TIMEOUT 30
    )

    add_test(
        NAME connext_shapes_xcdr_integration
        COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_shapes_xcdr_integration.sh"
            "$<TARGET_FILE:connext_shapes_xcdr_subscriber>"
            "$<TARGET_FILE:connext_shapes_xcdr_publisher>"
    )
    set_tests_properties(connext_shapes_xcdr_integration PROPERTIES
        WORKING_DIRECTORY "${shapes_xcdr_example_dir}"
        PASS_REGULAR_EXPRESSION
            "DDS XCDR subscriber received 20 ShapeTypeExtended samples"
        TIMEOUT 45
    )
endif()
