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

include_guard(GLOBAL)
include(ConnextDdsCodegen)

function(rti_holoscan_add_idl)
    set(options)
    set(one_value_args TARGET IDL)
    set(multi_value_args INCLUDE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "rti_holoscan_add_idl: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_TARGET)
        message(FATAL_ERROR "rti_holoscan_add_idl requires TARGET")
    endif()
    if(NOT ARG_IDL)
        message(FATAL_ERROR "rti_holoscan_add_idl requires IDL")
    endif()
    if(TARGET "${ARG_TARGET}")
        message(FATAL_ERROR "rti_holoscan_add_idl: target '${ARG_TARGET}' already exists")
    endif()

    get_filename_component(idl_file "${ARG_IDL}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT EXISTS "${idl_file}")
        message(FATAL_ERROR "rti_holoscan_add_idl: IDL file does not exist: ${idl_file}")
    endif()

    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${ARG_TARGET}")
    string(MAKE_C_IDENTIFIER "${ARG_TARGET}" variable_prefix)
    string(TOUPPER "${variable_prefix}" variable_prefix)

    connextdds_rtiddsgen_run(
        LANG C++11
        OUTPUT_DIRECTORY "${generated_dir}"
        IDL_FILE "${idl_file}"
        INCLUDE_DIRS ${ARG_INCLUDE_DIRS}
        VAR "${variable_prefix}"
    )

    connextdds_sanitize_language(LANG C++11 VAR language_variable)
    set(generated_sources "${${variable_prefix}_${language_variable}_SOURCES}")
    set(generated_headers "${${variable_prefix}_${language_variable}_HEADERS}")

    add_library("${ARG_TARGET}" STATIC
        ${generated_sources}
        ${generated_headers}
    )
    target_compile_features("${ARG_TARGET}" PUBLIC cxx_std_20)
    target_include_directories("${ARG_TARGET}"
        PUBLIC
            "$<BUILD_INTERFACE:${generated_dir}>"
    )
    target_link_libraries("${ARG_TARGET}" PUBLIC RTIConnextDDS::cpp2_api)
endfunction()
