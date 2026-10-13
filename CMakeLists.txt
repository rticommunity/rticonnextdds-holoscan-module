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

cmake_minimum_required(VERSION 3.30)
project(rticonnextdds_holoscan_module VERSION 0.1.0
    DESCRIPTION "RTI Connext DDS integration for NVIDIA Holoscan" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED BUILD_ALL)
  if(PROJECT_IS_TOP_LEVEL)
    set(BUILD_ALL ON)
  else()
    set(BUILD_ALL OFF)
  endif()
endif()

if(NOT COMMAND add_holohub_operator OR NOT COMMAND add_holohub_application)
  include(cmake/HoloHubConfigHelpers.cmake)
endif()

find_package(Threads REQUIRED)
find_package(holoscan 5.0.0 REQUIRED CONFIG)
if(NOT DEFINED ENV{NDDSHOME} OR "$ENV{NDDSHOME}" STREQUAL "")
  message(FATAL_ERROR "NDDSHOME must point to an RTI Connext DDS installation")
endif()
list(PREPEND CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
find_package(RTIConnextDDS 7.7.0 REQUIRED)

include(CTest)
option(RTI_HOLOSCAN_ENABLE_VISUAL_TESTS
  "Register the Docker/Xvfb Holoviz visual integration test" OFF)

include(cmake/RTIConnextDDSGenerateIdl.cmake)
add_subdirectory(applications)
add_subdirectory(operators)

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING
   AND APP_shapes_demo_flatbuffers AND APP_connext_shapes_holoviz)
  add_subdirectory(tests)
endif()
