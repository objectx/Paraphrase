
cmake_minimum_required (VERSION 3.15)

set (libname_ extFactor)
    add_library (${libname_} SHARED extFactor.cpp)
    target_link_libraries (${libname_}
                           PRIVATE ${PROJECT_NAME}-common
                                   ${PROJECT_NAME}-no-rtti
                                   PP)

set (libname_ extLP)
    add_library (${libname_} SHARED extFactor.cpp)
    target_link_libraries (${libname_}
                           PRIVATE ${PROJECT_NAME}-common
                                   ${PROJECT_NAME}-no-rtti
                                   PP)
