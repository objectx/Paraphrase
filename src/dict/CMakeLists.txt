
cmake_minimum_required (VERSION 3.15)

set (libname_ dict)

set (srcs_
     dictWord.cpp
     dictObject.cpp
     dictStack.cpp
     dictControl.cpp
     dictIO.cpp
     dictMath.cpp
     dictString.cpp
     dictArray.cpp
     dictList.cpp
     dictParallel.cpp
     dictOptimize.cpp)

add_library (${libname_} OBJECT ${srcs_})
target_link_libraries (${libname_} PRIVATE
                       Boost::headers
                       ${PROJECT_NAME}-common ${PROJECT_NAME}-no-rtti)

add_subdirectory (ext)
