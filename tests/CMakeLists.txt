# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

add_executable(holoscan_connext_baseline_smoke baseline_smoke.cpp)
target_link_libraries(holoscan_connext_baseline_smoke PRIVATE holoscan::core
  RTIConnextDDS::cpp2_api Threads::Threads)
add_test(NAME holoscan_connext_baseline_smoke COMMAND holoscan_connext_baseline_smoke)
set_tests_properties(holoscan_connext_baseline_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "Holoscan 5 and RTI Connext DDS baseline passed" TIMEOUT 30)

add_executable(connext_shapes_adapter_test shapes_adapter_test.cpp)
add_dependencies(connext_shapes_adapter_test shapes_demo_payload_generated)
target_include_directories(connext_shapes_adapter_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../applications/shapes_demo_flatbuffers"
  "${CMAKE_BINARY_DIR}/applications/shapes_demo_flatbuffers/generated")
target_link_libraries(connext_shapes_adapter_test PRIVATE holoscan::core holoscan::schema
  shapes_demo_dds_types)
add_test(NAME connext_shapes_adapter_test COMMAND connext_shapes_adapter_test)
set_tests_properties(connext_shapes_adapter_test PROPERTIES
  PASS_REGULAR_EXPRESSION "Shapes adapter preserved all ShapeTypeExtended fields" TIMEOUT 30)

add_executable(connext_shapes_conversion_benchmark shapes_conversion_benchmark.cpp)
add_dependencies(connext_shapes_conversion_benchmark shapes_demo_payload_generated)
target_include_directories(connext_shapes_conversion_benchmark PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../applications/shapes_demo_flatbuffers"
  "${CMAKE_CURRENT_SOURCE_DIR}/../applications/shapes_demo_xcdr"
  "${CMAKE_BINARY_DIR}/applications/shapes_demo_flatbuffers/generated")
target_link_libraries(connext_shapes_conversion_benchmark PRIVATE holoscan::core holoscan::schema
  shapes_demo_dds_types)
add_test(NAME connext_shapes_conversion_benchmark_smoke
  COMMAND connext_shapes_conversion_benchmark --iterations 1000)
set_tests_properties(connext_shapes_conversion_benchmark_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "xcdr_overhead_ratio=" TIMEOUT 30)

set(shapes_demo_flatbuffers_example_dir "${CMAKE_BINARY_DIR}/applications/shapes_demo_flatbuffers")
set(shapes_demo_xcdr_example_dir "${CMAKE_BINARY_DIR}/applications/shapes_demo_xcdr")

function(add_dds_pair_test name subscriber publisher pattern domain working_directory)
  add_test(NAME "${name}" COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/run_dds_pair.sh"
    "${subscriber}" "${publisher}" "${pattern}")
  set_tests_properties("${name}" PROPERTIES WORKING_DIRECTORY "${working_directory}"
    PASS_REGULAR_EXPRESSION "${pattern}" TIMEOUT 45
    ENVIRONMENT "HOLOSCAN_DDS_DOMAIN_ID=${domain}")
endfunction()

add_dds_pair_test(connext_shapes_demo_flatbuffers_integration
  "$<TARGET_FILE:connext_shapes_demo_flatbuffers_subscriber>" "$<TARGET_FILE:connext_shapes_demo_flatbuffers_publisher>"
  "DDS Shapes subscriber received 20 ShapeTypeExtended samples" 11
  "${shapes_demo_flatbuffers_example_dir}")
add_dds_pair_test(connext_shapes_demo_xcdr_integration
  "$<TARGET_FILE:connext_shapes_demo_xcdr_subscriber>" "$<TARGET_FILE:connext_shapes_demo_xcdr_publisher>"
  "DDS XCDR subscriber received 20 ShapeTypeExtended samples" 12
  "${shapes_demo_xcdr_example_dir}")
