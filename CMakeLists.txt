cmake_minimum_required(VERSION 3.10)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(foxdbg)

add_subdirectory(extern)

add_library(foxdbg STATIC
    lib/foxdbg.c
    lib/foxdbg_buffer.c

    lib/foxdbg_thread.cpp
    lib/foxdbg_protocol.cpp
)

target_link_libraries(foxdbg PRIVATE
    websockets
    ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/turbojpeg-static.lib
)

target_include_directories(foxdbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/libwebsockets/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/libjpeg-turbo/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable(foxdbg_test 
    examples/c/main.cpp
)

target_link_libraries(foxdbg_test PRIVATE
    foxdbg
)

target_include_directories(foxdbg_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
)