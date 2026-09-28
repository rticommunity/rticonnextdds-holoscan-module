# *******************************************************************************
#  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
#  RTI grants Licensee a license to use, modify, compile, and create derivative
#  works of the Software. Licensee has the right to distribute object form only
#  for use with RTI products. The Software is provided "as is", with no warranty
#  of any type, including any warranty for fitness for any purpose. RTI is under no
#  obligation to maintain or support the Software. RTI shall not be liable for any
#  incidental or consequential damages arising out of the use or inability to use
#  the software.
# *******************************************************************************/

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

set(shapes_demo_flatbuffers_example_dir "${CMAKE_BINARY_DIR}/applications/shapes_demo_flatbuffers")
set(connext_shapes_holoviz_test_image "rticonnextdds-holoscan-module-rticonnextdds-holoscan-module:ea-2"
  CACHE STRING "Container image used by the isolated HoloViz visual CTest")
set(connext_shapes_holoviz_capture_dir "build/rticonnextdds-holoscan-module/test-artifacts/connext_shapes_holoviz")

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

if(RTI_HOLOSCAN_ENABLE_VISUAL_TESTS)
  add_test(NAME connext_shapes_holoviz_visual_integration
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/run_connext_shapes_holoviz_visual.sh"
      "${connext_shapes_holoviz_test_image}" "${connext_shapes_holoviz_capture_dir}")
  set_tests_properties(connext_shapes_holoviz_visual_integration PROPERTIES
    PASS_REGULAR_EXPRESSION "connext_shapes_holoviz visual DDS test passed"
    TIMEOUT 45
    LABELS "integration;visual;docker")
endif()
