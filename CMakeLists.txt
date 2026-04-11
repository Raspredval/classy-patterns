cmake_minimum_required(VERSION 3.16)

project(classy-patterns LANGUAGES CXX VERSION 0.3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CXX_WARNINGS "-Wall" "-Wextra" "-Wconversion" "-Werror")

set(ASSETS_DIR  "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_LINK "${CMAKE_BINARY_DIR}/assets")
file (CREATE_LINK ${ASSETS_DIR} ${ASSETS_LINK} SYMBOLIC)

set(LIBRARY_HEADERS
    "include/Patterns.hpp"
    "include/Pattern.hpp"
    "include/Grammar.hpp"
    "include/Match.hpp")
set(LIBRARY_SOURCES
    "source/Patterns.cpp"
    "source/Pattern.cpp"
    "source/Grammar.cpp"
    "source/Match.cpp")

add_library(classy-patterns-shared
    SHARED
        ${LIBRARY_SOURCES})
set_target_properties(classy-patterns-shared
    PROPERTIES
        OUTPUT_NAME         "classy-patterns"
        OUTPUT_NAME_DEBUG   "classy-patterns-debug")
target_compile_options(classy-patterns-shared
    PRIVATE
        ${CXX_WARNINGS})
target_include_directories(classy-patterns-shared
    PRIVATE
        "include/")

add_library(classy-patterns-static
    STATIC
        ${LIBRARY_SOURCES})
set_target_properties(classy-patterns-static
    PROPERTIES
        OUTPUT_NAME         "classy-patterns"
        OUTPUT_NAME_DEBUG   "classy-patterns-debug")
target_compile_options(classy-patterns-static
    PRIVATE
        ${CXX_WARNINGS})
target_include_directories(classy-patterns-static
    PRIVATE
        "include/")

install(
    FILES       ${LIBRARY_HEADERS}
    DESTINATION "include/classy-patterns"
    COMPONENT   classy-patterns-headers)
install(
    TARGETS     classy-patterns-shared classy-patterns-static
    DESTINATION "lib/"
    COMPONENT   classy-patterns-binaries)

include(CPack)

set(CPACK_PACKAGE_NAME "classy-patterns")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCTIPTION_SUMMARY "PEG-like text parsing library, heavily inspired by lpeg")
set(CPACK_INSTALL_PREFIX "/usr/local/")

cpack_add_component(classy-patterns-headers
    DISPLAY "Header files")
cpack_add_component(classy-patterns-binaries
    DISPLAY "binary files")

add_executable(tests
    "tests/json.cpp")
target_compile_options(tests
    PRIVATE
        ${CXX_WARNINGS})
target_include_directories(tests
    PRIVATE
        "include/")
target_link_libraries(tests
    PRIVATE
        classy-streams
        classy-patterns-shared)