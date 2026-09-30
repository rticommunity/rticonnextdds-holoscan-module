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

rti_holoscan_add_idl(TARGET shapes_demo_dds_types IDL "$ENV{NDDSHOME}/resource/idl/ShapeType.idl")
set(shapes_demo_flatbuffers_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
holoscan_add_flatbuffer_schema(TARGET shapes_demo_payload_generated
  SCHEMAS shape.fbs INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_DIR "${shapes_demo_flatbuffers_generated_dir}" BFBS_EMBED
  TRAITS_HEADER shape_schema_traits.hpp TRAITS_SCHEMA shape.fbs
  TRAITS_ROOT_TYPE rti::holoscan::shapes::Shape
  TRAITS_NATIVE_TYPE rti::holoscan::shapes::ShapeT
  TRAITS_BINARY_SCHEMA_TYPE rti::holoscan::shapes::ShapeBinarySchema
  TRAITS_PACKAGE_NAMESPACE rti_holoscan_shapes::package
  TRAITS_CANONICAL_NAME rti.holoscan.shapes.Shape TRAITS_SCHEMA_VERSION 1
  TRAITS_MAX_SERIALIZED_SIZE 512)

set(shapes_demo_flatbuffers_example_dir "${CMAKE_CURRENT_BINARY_DIR}")
configure_file(HoloscanConnextQos.xml "${shapes_demo_flatbuffers_example_dir}/HoloscanConnextQos.xml" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/scripts/require_rti_license.sh" "${shapes_demo_flatbuffers_example_dir}/require_rti_license.sh" COPYONLY)
foreach(role IN ITEMS publisher subscriber)
  add_executable(connext_shapes_demo_flatbuffers_${role} ${role}.cpp)
  add_dependencies(connext_shapes_demo_flatbuffers_${role} shapes_demo_payload_generated)
  target_include_directories(connext_shapes_demo_flatbuffers_${role} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${shapes_demo_flatbuffers_generated_dir}")
  target_link_libraries(connext_shapes_demo_flatbuffers_${role} PRIVATE RTIConnextDDSHoloscan::dds
    holoscan::schema shapes_demo_dds_types)
endforeach()
