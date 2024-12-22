cmake_minimum_required(VERSION 3.16)
project(cadical)

include(ExternalProject)

# Paths
set(CADICAL_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(CADICAL_BUILD_DIR ${PROJECT_SOURCE_DIR}/build)
set(CADICAL_INSTALL_DIR ${CMAKE_BINARY_DIR}/cadical_install)
message(STATUS "CADICAL_SOURCE_DIR: ${CADICAL_SOURCE_DIR}")
message(STATUS "CADICAL_BUILD_DIR: ${CADICAL_BUILD_DIR}")
message(STATUS "CADICAL_INSTALL_DIR: ${CADICAL_INSTALL_DIR}")

# Configure and build using ExternalProject
ExternalProject_Add(
    cadical_project
    SOURCE_DIR ${CADICAL_SOURCE_DIR}
    BINARY_DIR ${CADICAL_BUILD_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir ${CADICAL_SOURCE_DIR} ./configure
    BUILD_COMMAND ${CMAKE_COMMAND} -E chdir ${CADICAL_SOURCE_DIR}/build make
    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CADICAL_INSTALL_DIR} &&
                    ${CMAKE_COMMAND} -E copy ${CADICAL_BUILD_DIR}/libcadical.a ${CADICAL_INSTALL_DIR}
)

# Create an interface target for cadical
add_library(cadical STATIC IMPORTED GLOBAL)
set_target_properties(cadical PROPERTIES
    IMPORTED_LOCATION ${CADICAL_INSTALL_DIR}/libcadical.a
    INTERFACE_INCLUDE_DIRECTORIES ${CADICAL_SOURCE_DIR}/src
)

# Ensure that the cadical target depends on the build process
add_dependencies(cadical cadical_project)