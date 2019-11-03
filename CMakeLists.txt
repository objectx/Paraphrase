
cmake_minimum_required (VERSION 3.15)

project (Paraphrase CXX)

cmake_policy (SET CMP0076 NEW)  # Uses a new `target_sources` behavior.

function (set_if_not_defined_ var_)
    if (NOT DEFINED ${var_})
        set (${var_} ${ARGN} PARENT_SCOPE)
    endif ()
endfunction ()

set_if_not_defined_ (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_if_not_defined_ (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set_if_not_defined_ (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory (src)
