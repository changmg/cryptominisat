cmake_minimum_required(VERSION 3.16)
project(cadical)

include(ExternalProject)

# Path to the source directory
set(CADICAL_SOURCE_DIR ${PROJECT_SOURCE_DIR})
message(STATUS "CADICAL_SOURCE_DIR: ${CADICAL_SOURCE_DIR}")

# Path to the build directory
set(CADICAL_BUILD_DIR ${CMAKE_BINARY_DIR}/cadical_build)

# Configure and build using ExternalProject
ExternalProject_Add(
    cadical_project
    SOURCE_DIR ${CADICAL_SOURCE_DIR}
    BINARY_DIR ${CADICAL_BUILD_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir ${CADICAL_SOURCE_DIR} ./configure
    BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${CADICAL_SOURCE_DIR}/build make
    INSTALL_COMMAND ""
)

# Optional: create a custom target to build cadical
add_custom_target(cadical ALL
    DEPENDS cadical_project
)