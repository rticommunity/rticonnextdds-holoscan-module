rti_holoscan_add_idl(TARGET connext_shapes_holoviz_dds_types IDL "$ENV{NDDSHOME}/resource/idl/ShapeType.idl")
set(connext_shapes_holoviz_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
holoscan_add_flatbuffer_schema(TARGET connext_shapes_holoviz_payload_generated
  SCHEMAS shape.fbs INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_DIR "${connext_shapes_holoviz_generated_dir}" BFBS_EMBED
  TRAITS_HEADER shape_schema_traits.hpp TRAITS_SCHEMA shape.fbs
  TRAITS_ROOT_TYPE rti::holoscan::shapes::Shape
  TRAITS_NATIVE_TYPE rti::holoscan::shapes::ShapeT
  TRAITS_BINARY_SCHEMA_TYPE rti::holoscan::shapes::ShapeBinarySchema
  TRAITS_PACKAGE_NAMESPACE rti_holoscan_shapes::package
  TRAITS_CANONICAL_NAME rti.holoscan.shapes.Shape TRAITS_SCHEMA_VERSION 1
  TRAITS_MAX_SERIALIZED_SIZE 512)
configure_file(HoloscanConnextQos.xml "${CMAKE_CURRENT_BINARY_DIR}/HoloscanConnextQos.xml" COPYONLY)

add_executable(connext_shapes_holoviz main.cpp)
add_dependencies(connext_shapes_holoviz connext_shapes_holoviz_payload_generated)
target_include_directories(connext_shapes_holoviz PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${connext_shapes_holoviz_generated_dir}")
target_link_libraries(connext_shapes_holoviz PRIVATE
  RTIConnextDDSHoloscan::dds holoscan::schema holoscan::operators::holoviz connext_shapes_holoviz_dds_types)
