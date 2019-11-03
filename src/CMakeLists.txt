
cmake_minimum_required (VERSION 3.15)

cmake_policy (SET CMP0076 NEW)  # Uses a new `target_sources` behavior.

set (Boost_USE_STATIC_RUNTIME OFF)
set (Boost_NO_BOOST_CMAKE ON)
find_package (Boost CONFIG REQUIRED
              COMPONENTS program_options timer)

if (NOT Boost_FOUND)
    message (SEND_ERROR "Boost is required")
endif ()

find_library (libreadline_ NAME readline HINTS /usr/local/opt/readline)

set (tgt_ ${PROJECT_NAME}-no-rtti)
    add_library (${tgt_} INTERFACE)
    target_compile_options (${tgt_} INTERFACE -fno-rtti)

set (tgt_ ${PROJECT_NAME}-common)
    add_library (${tgt_} INTERFACE)
    target_compile_features (${tgt_} INTERFACE cxx_std_17)
    target_compile_definitions (${tgt_} INTERFACE
                                USE_NOEXCEPT=1 USE_PTHREAD=1
                                $<IF:$<CONFIG:Release>,NDEBUG=1,DEBUG=1>)
    target_include_directories (${tgt_} INTERFACE inc)

add_subdirectory (dict)
add_subdirectory (lang)

set (tgt_ para)
    add_executable (${tgt_} lang/para.cpp)
    target_link_libraries (para
                           PRIVATE PP
                                   ${PROJECT_NAME}-common
                                   ${PROJECT_NAME}-no-rtti
                                   Boost::headers
                                   Boost::program_options
                                   Boost::timer)
    if (libreadline_)
        message (STATUS "`readline` found… use it.")
        target_compile_definitions (${tgt_} PRIVATE USE_READLINE=1)
        target_link_libraries (${tgt_} PRIVATE ${libreadline_})
    else ()
        message (STATUS "Missing `readline`.")
    endif ()
