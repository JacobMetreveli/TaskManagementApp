cmake_minimum_required(VERSION 3.29)
project(TaskManagement C)

set(CMAKE_C_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(XCB REQUIRED xcb)

include_directories(
        ${OPENGL_INCLUDE_DIRS}
        /usr/local/include
        ${XCB_INCLUDE_DIRS}
)

add_executable(TaskManagement main.c)

target_link_libraries(
        TaskManagement
        PRIVATE
            OpenGL::GL
            glfw
            /usr/local/lib/libleif.a
            /usr/local/lib/libclipboard.a
            m
            dl
            Threads::Threads
            ${XCB_LIBRARIES}
)