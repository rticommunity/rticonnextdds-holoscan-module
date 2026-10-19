rti_holoscan_add_idl(TARGET connext_shapes_holoviz_dds_types IDL "$ENV{NDDSHOME}/resource/idl/ShapeType.idl")
set(connext_shapes_holoviz_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
holoscan_add_flatbuffer_schema(TARGET connext_shapes_holoviz_payload_generated
  SCHEMAS shape.fbs INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_DIR "${connext_shapes_holoviz_generated_dir}" BFBS_EMBED
  MAX_SERIALIZED_SIZE 512)
configure_file(HoloscanConnextQos.xml "${CMAKE_CURRENT_BINARY_DIR}/HoloscanConnextQos.xml" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/scripts/require_rti_license.sh" "${CMAKE_CURRENT_BINARY_DIR}/require_rti_license.sh" COPYONLY)

add_executable(connext_shapes_holoviz main.cpp)
add_dependencies(connext_shapes_holoviz connext_shapes_holoviz_payload_generated)
target_include_directories(connext_shapes_holoviz PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${connext_shapes_holoviz_generated_dir}")
target_link_libraries(connext_shapes_holoviz PRIVATE
  RTIConnextDDSHoloscan::dds holoscan::schema holoscan::visualization::op_holoviz connext_shapes_holoviz_dds_types)
