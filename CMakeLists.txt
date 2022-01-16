# Require at least version 3.14
cmake_minimum_required(VERSION 3.14)

project("Rat In a Tube"
    VERSION 0.1.0
    DESCRIPTION "HCE script compiler"
    LANGUAGES C CXX
)

add_library(riat
    src/riat.c
    src/riat_token.c
)
set_property(TARGET riat PROPERTY C_STANDARD 11)

add_executable(riat-test
    main.cpp
)
set_property(TARGET riat-test PROPERTY C_STANDARD 11)
set_property(TARGET riat-test PROPERTY CXX_STANDARD 20)
target_link_libraries(riat-test riat)
