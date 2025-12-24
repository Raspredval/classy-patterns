cmake_minimum_required(VERSION 3.16)

project(classy-patterns LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(classy-patterns
    INTERFACE)
file(GLOB LIBRARY_HEADERS "${CMAKE_SOURCE_DIR}/include/*.hpp")
target_include_directories(classy-patterns
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

install(
    FILES ${LIBRARY_HEADERS}
    DESTINATION "include/classy-patterns"
    COMPONENT classy-patterns-headers)

include(CPack)

set(CPACK_PACKAGE_NAME "classy-patterns")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCTIPTION_SUMMARY "an attempt to 'fix' stl iostreams, add basic network capability")
set(CPACK_INSTALL_PREFIX "/usr/local/")

cpack_add_component(classy-patterns-headers
    DISPLAY "Header files")

include(CTest)

set(CXX_WARNINGS "-Wall" "-Wextra" "-Wconversion" "-Werror")

add_executable(tests
    "source/tests.cpp")
target_compile_options(tests
    PRIVATE
        ${CXX_WARNINGS})
target_include_directories(tests
    PRIVATE
        "include/")

add_test(
    NAME    tests
    COMMAND tests)