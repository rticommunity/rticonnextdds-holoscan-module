# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

set(shapes_demo_xcdr_example_dir "${CMAKE_CURRENT_BINARY_DIR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../shapes_demo_flatbuffers/HoloscanConnextQos.xml"
  "${shapes_demo_xcdr_example_dir}/HoloscanConnextQos.xml" COPYONLY)
foreach(role IN ITEMS publisher subscriber)
  add_executable(connext_shapes_demo_xcdr_${role} ${role}.cpp)
  add_dependencies(connext_shapes_demo_xcdr_${role} shapes_demo_payload_generated)
  target_include_directories(connext_shapes_demo_xcdr_${role} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../shapes_demo_flatbuffers" "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/../shapes_demo_flatbuffers/generated")
  target_link_libraries(connext_shapes_demo_xcdr_${role} PRIVATE RTIConnextDDSHoloscan::dds
    holoscan::schema shapes_demo_dds_types)
endforeach()
