# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.30)
project(rticonnextdds_holoscan_module VERSION 0.1.0
    DESCRIPTION "RTI Connext DDS integration for NVIDIA Holoscan" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(holoscan 5.0.0 REQUIRED CONFIG)
if(NOT DEFINED ENV{NDDSHOME} OR "$ENV{NDDSHOME}" STREQUAL "")
  message(FATAL_ERROR "NDDSHOME must point to an RTI Connext DDS installation")
endif()
list(PREPEND CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
find_package(RTIConnextDDS 7.7.0 REQUIRED)

include(CTest)
include(cmake/RTIConnextDDSGenerateIdl.cmake)

add_library(rti_connext_dds_operators INTERFACE)
add_library(RTIConnextDDSHoloscan::dds ALIAS rti_connext_dds_operators)
target_compile_features(rti_connext_dds_operators INTERFACE cxx_std_20)
target_include_directories(rti_connext_dds_operators INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/operators/dds/include>")
target_link_libraries(rti_connext_dds_operators INTERFACE
  holoscan::core RTIConnextDDS::cpp2_api Threads::Threads)

add_subdirectory(applications/shapes_demo_flatbuffers)
add_subdirectory(applications/shapes_demo_xcdr)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
