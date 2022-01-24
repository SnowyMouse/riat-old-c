# Require at least version 3.14
cmake_minimum_required(VERSION 3.14)

project("Rat In a Tube"
    VERSION 0.1.0
    DESCRIPTION "HCE script compiler"
    LANGUAGES C CXX
)

find_package(Python3 REQUIRED)

add_library(riat
    src/riat.c
    src/definition/definition_array.h
    src/definition/definition.c
    src/riat_token.c
    src/riat_tree.c
    src/riat_value_type.c

    src/riat_strdup.c
)
set_property(TARGET riat PROPERTY C_STANDARD 11)

add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_array.h"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.json" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_array.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.json"
)

option(RIAT_BUILD_EXAMPLES "Compile the examples?" OFF)
if(RIAT_BUILD_EXAMPLES)
    add_executable(riat-validator-cpp
        examples/validator-cpp.cpp
    )
    set_property(TARGET riat-validator-cpp PROPERTY CXX_STANDARD 11)
    target_link_libraries(riat-validator-cpp riat)
endif()

option(BUILD_SHARED_LIBS "Build with shared libs" OFF)
