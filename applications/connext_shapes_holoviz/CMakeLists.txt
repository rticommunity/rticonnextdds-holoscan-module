# *******************************************************************************
#  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
#  RTI Examples License.
# *******************************************************************************/

cmake_minimum_required(VERSION 3.24)
project(connext_shapes_holoviz LANGUAGES CXX)

set(connext_shapes_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
connextdds_rtiddsgen_run(
  IDL_FILE "$ENV{RTI_CONNEXT_DDS_DIR}/resource/idl/ShapeType.idl"
  OUTPUT_DIRECTORY "${connext_shapes_generated_dir}"
  LANG C++11
  VAR connext_shapes)

add_library(connext_shapes_types STATIC ${connext_shapes_CXX11_SOURCES})
target_include_directories(connext_shapes_types PUBLIC
  "$<BUILD_INTERFACE:${connext_shapes_generated_dir}>")
target_link_libraries(connext_shapes_types PUBLIC RTIConnextDDS::cpp2_api)

add_executable(connext_shapes_holoviz main.cpp)
target_link_libraries(connext_shapes_holoviz PRIVATE
  holoscan::core
  holoscan::ops::holoviz
  connext_shapes_types)
target_include_directories(connext_shapes_holoviz PRIVATE
  "$<BUILD_INTERFACE:${connext_shapes_generated_dir}>")

configure_file(HoloscanConnextQos.xml "${CMAKE_BINARY_DIR}/HoloscanConnextQos.xml" COPYONLY)
add_dependencies(connext_shapes_holoviz connext_shapes_types)
