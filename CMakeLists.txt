cmake_minimum_required(VERSION 3.10)

project(system VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_DEBUG_POSTFIX d)

# Compiler flags
set(CXX_FLAGS
    -g
    -ggdb
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -march=native
    -rdynamic
    -O0 -Wall
    -DDEBUG
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Output directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/debug/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/debug/lib)

# Source files
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cxx
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/inc)
include_directories(${CMAKE_SOURCE_DIR}/extern_inc)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/lib/debug)

# Build targets
add_library(system STATIC
    ${SRC_FILES}
)

add_executable(directory_test
    ${CMAKE_SOURCE_DIR}/main/directory_test.cc
    ${SRC_FILES}
)

add_executable(regression_test
    ${CMAKE_SOURCE_DIR}/main/regression_test.cc
    ${SRC_FILES}
)

add_executable(dns
    ${CMAKE_SOURCE_DIR}/main/dns.cc
    ${SRC_FILES}
)

add_executable(echo_client_test
    ${CMAKE_SOURCE_DIR}/main/echo_client_test.cc
    ${SRC_FILES}
)

add_executable(echo_server_test
    ${CMAKE_SOURCE_DIR}/main/echo_server_test.cc
    ${SRC_FILES}
)

add_executable(file_test
    ${CMAKE_SOURCE_DIR}/main/file_test.cc
    ${SRC_FILES}
)

add_executable(mutex_test
    ${CMAKE_SOURCE_DIR}/main/mutex_test.cc
    ${SRC_FILES}
)

add_executable(threadpool_test
    ${CMAKE_SOURCE_DIR}/main/threadpool_test.cc
    ${SRC_FILES}
)

# Include dirs for target system
target_include_directories(system PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(system PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(system PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target directory_test
target_include_directories(directory_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(directory_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(directory_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target regression_test
target_include_directories(regression_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(regression_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(regression_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target dns
target_include_directories(dns PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(dns PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(dns PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target echo_client_test
target_include_directories(echo_client_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(echo_client_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(echo_client_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target echo_server_test
target_include_directories(echo_server_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(echo_server_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(echo_server_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target file_test
target_include_directories(file_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(file_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(file_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target mutex_test
target_include_directories(mutex_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(mutex_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(mutex_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Include dirs for target threadpool_test
target_include_directories(threadpool_test PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_include_directories(threadpool_test PRIVATE ${CMAKE_SOURCE_DIR}/extern_inc)
target_include_directories(threadpool_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Legacy dependencies
target_link_libraries(system
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(directory_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(regression_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(dns
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(echo_client_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(echo_server_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(file_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(mutex_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)
target_link_libraries(threadpool_test
    ncurses++
    form
    menu
    panel
    ncurses
    basic
    pthread
)

