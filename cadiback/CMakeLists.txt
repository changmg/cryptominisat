cmake_minimum_required(VERSION 3.16)
project(cadiback LANGUAGES CXX)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -O3 -std=c++17 -DNDEBUG")

# Paths
set(CADICAL_SRC_DIR ${CMAKE_SOURCE_DIR}/../cadical/src)
set(CADICAL_BUILD_DIR ${CMAKE_SOURCE_DIR}/../cadical/build)

# Generate the config.hpp file
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/config.hpp
    COMMAND ${PROJECT_SOURCE_DIR}/generate > ${CMAKE_BINARY_DIR}/config.hpp
    DEPENDS ${PROJECT_SOURCE_DIR}/generate
    COMMENT "Generating config.hpp"
)

# Add the custom target to generate the header file
add_custom_target(generate_config DEPENDS ${CMAKE_BINARY_DIR}/config.hpp)

# Add the object file target
add_library(cadiback_obj OBJECT cadiback.cpp ${CMAKE_BINARY_DIR}/config.hpp)
target_include_directories(cadiback_obj PRIVATE ${CADICAL_SRC_DIR})

# Create a static library
add_library(cadiback_static STATIC $<TARGET_OBJECTS:cadiback_obj>)
add_dependencies(cadiback_static generate_config)

# Create a shared library
add_library(cadiback_shared SHARED $<TARGET_OBJECTS:cadiback_obj>)
target_link_libraries(cadiback_shared PRIVATE ${CADICAL_BUILD_DIR}/libcadical.a)
add_dependencies(cadiback_shared generate_config)

# Install the libraries (optional)
install(TARGETS cadiback_static cadiback_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)