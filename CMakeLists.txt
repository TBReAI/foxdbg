cmake_minimum_required(VERSION 3.10)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(foxdbg)

add_subdirectory(extern)

option(FOXDBG_BUILD_TESTS "Build tests" OFF)

add_library(foxdbg STATIC
    lib/foxdbg.c
    lib/foxdbg_buffer.c

    lib/foxdbg_thread.cpp
    lib/foxdbg_protocol.cpp
)

add_dependencies(foxdbg libjpeg-turbo)

if (WIN32)
    set(FOXDBG_LIBS 
        websockets
        ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/turbojpeg-static.lib
    )
else()
    set(FOXDBG_LIBS 
        websockets
        ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib64/libjpeg.a
        ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib64/libturbojpeg.a
    )
endif()

target_link_libraries(foxdbg PRIVATE
    ${FOXDBG_LIBS}
)

if(UNIX AND NOT APPLE)
    target_link_options(foxdbg PRIVATE -rdynamic)
endif()


target_include_directories(foxdbg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/libwebsockets/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/libjpeg-turbo/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

if (FOXDBG_BUILD_TESTS)
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
endif()