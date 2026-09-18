set(connext_shapes_holoviz_generated_dir "${CMAKE_BINARY_DIR}/applications/shapes_demo_flatbuffers/generated")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../shapes_demo_flatbuffers/HoloscanConnextQos.xml" "${CMAKE_CURRENT_BINARY_DIR}/HoloscanConnextQos.xml" COPYONLY)

add_executable(connext_shapes_holoviz main.cpp)
add_dependencies(connext_shapes_holoviz shapes_demo_payload_generated)
target_include_directories(connext_shapes_holoviz PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../shapes_demo_flatbuffers"
  "${connext_shapes_holoviz_generated_dir}")
target_link_libraries(connext_shapes_holoviz PRIVATE
  RTIConnextDDSHoloscan::dds holoscan::schema holoscan::operators::holoviz shapes_demo_dds_types)
