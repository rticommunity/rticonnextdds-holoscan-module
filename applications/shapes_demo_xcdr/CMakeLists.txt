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

rti_holoscan_add_idl(TARGET shapes_demo_xcdr_dds_types IDL "$ENV{NDDSHOME}/resource/idl/ShapeType.idl")
configure_file(HoloscanConnextQos.xml "${CMAKE_CURRENT_BINARY_DIR}/HoloscanConnextQos.xml" COPYONLY)

foreach(role IN ITEMS publisher subscriber)
  add_executable(connext_shapes_demo_xcdr_${role} ${role}.cpp)
  target_include_directories(connext_shapes_demo_xcdr_${role} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(connext_shapes_demo_xcdr_${role} PRIVATE
    RTIConnextDDSHoloscan::dds shapes_demo_xcdr_dds_types)
endforeach()
