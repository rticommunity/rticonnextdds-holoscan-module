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

add_library(rti_connext_dds_operators INTERFACE)
add_library(RTIConnextDDSHoloscan::dds ALIAS rti_connext_dds_operators)
target_compile_features(rti_connext_dds_operators INTERFACE cxx_std_20)
target_include_directories(rti_connext_dds_operators INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(rti_connext_dds_operators INTERFACE
  holoscan::core RTIConnextDDS::cpp2_api Threads::Threads)
