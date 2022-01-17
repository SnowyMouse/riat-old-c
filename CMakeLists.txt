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
    src/definition/definition.c
    src/riat_token.c
    src/riat_tree.c
    src/riat_value_type.c
)
set_property(TARGET riat PROPERTY C_STANDARD 11)

add_custom_command (OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.c"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.json" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.c"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition_codegen.py" "${CMAKE_CURRENT_SOURCE_DIR}/src/definition/definition.json"
)

add_executable(riat-test
    main.cpp
)
set_property(TARGET riat-test PROPERTY C_STANDARD 11)
set_property(TARGET riat-test PROPERTY CXX_STANDARD 20)
target_link_libraries(riat-test riat)
