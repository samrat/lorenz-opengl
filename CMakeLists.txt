cmake_minimum_required(VERSION 3.5)
project(lorenz_opengl)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -static")

add_custom_target(add_bin_dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")

set(SOURCE_FILES
        lorenz.c
        gl3w/gl3w.c)

set(SHADER_FILES
        ${CMAKE_SOURCE_DIR}/head.frag
        ${CMAKE_SOURCE_DIR}/head.vert
        ${CMAKE_SOURCE_DIR}/tail.frag
        ${CMAKE_SOURCE_DIR}/tail.vert)

if (WIN32)
    set(OPENGL_LIB opengl32)
endif (WIN32)

if (UNIX)
    if (!APPLE)
        set(OPENGL_LIB GL)
    else (APPLE)
        set(OPENGL_LIB OpenGL)
    endif (!APPLE)
endif (UNIX)

include_directories(gl3w)
include_directories(glfw-3.1.2/include)
add_subdirectory(glfw-3.1.2)

add_executable(lorenz_opengl ${SOURCE_FILES})
target_link_libraries(lorenz_opengl glfw ${OPENGL_LIB})

add_custom_command(TARGET lorenz_opengl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${SHADER_FILES} $<TARGET_FILE_DIR:lorenz_opengl>)
