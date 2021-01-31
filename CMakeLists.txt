cmake_minimum_required(VERSION 3.10)

project(drawtext-gl-freetype-harfbuzz)

# Compile GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
add_subdirectory(deps/glfw)

# drawtext
add_executable(drawtext 
    main.cpp
    deps/glad/src/glad.c
    )

target_include_directories(drawtext
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/include")

target_link_libraries(drawtext
    glfw)