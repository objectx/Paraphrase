
cmake_minimum_required (VERSION 3.15)

set (libname_ PP)

set (srcs_
     outer.cpp
     inner.cpp
     word.cpp
     context.cpp
     typedValue.cpp
     dictMan.cpp
     errorMessage.cpp
     optimizer.cpp)

add_library (${libname_}-static STATIC ${srcs_})
target_link_libraries (${libname_}-static
                       PRIVATE ${PROJECT_NAME}-common
                               ${PROJECT_NAME}-no-rtti
                               dict
                               Boost::headers)

add_library (${libname_} SHARED ${srcs_})
target_link_libraries (${libname_}
                       PRIVATE ${PROJECT_NAME}-common
                               ${PROJECT_NAME}-no-rtti
                               dict
                       PUBLIC Boost::headers)

# add_library (${libname_} SHARED ${srcs_})
# target_link_libraries (${libname_} PRIVATE
#                        ${PROJECT_NAME}-common
#                        ${PROJECT_NAME}-no-rtti
#                        Boost::headers)
