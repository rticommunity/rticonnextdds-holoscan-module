# *******************************************************************************
#  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
#  RTI Examples License.
# *******************************************************************************/

cmake_minimum_required(VERSION 3.24)
project(rticonnextdds_holoscan_module LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(holoscan 4.6 REQUIRED CONFIG
  PATHS /opt/nvidia/holoscan /workspace/holoscan-sdk/install)

if(NOT DEFINED ENV{NDDSHOME} OR "$ENV{NDDSHOME}" STREQUAL "")
  message(FATAL_ERROR "NDDSHOME must point to RTI Connext DDS 7.7.0")
endif()
list(PREPEND CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
find_package(RTIConnextDDS 7.7.0 REQUIRED)

include(CTest)
include(ConnextDdsCodegen)
add_subdirectory(applications/connext_shapes_holoviz)

if(BUILD_TESTING)
  add_test(NAME connext_shapes_holoviz_help
    COMMAND $<TARGET_FILE:connext_shapes_holoviz> --help)
  set_tests_properties(connext_shapes_holoviz_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Holoscan 4.6 RTI Shapes")

  add_test(NAME connext_shapes_holoviz_headless
    COMMAND $<TARGET_FILE:connext_shapes_holoviz>
      --headless --samples 40 --domain-id 42)
  set_tests_properties(connext_shapes_holoviz_headless PROPERTIES
    PASS_REGULAR_EXPRESSION "Published 40 black Square samples"
    TIMEOUT 30)
endif()
