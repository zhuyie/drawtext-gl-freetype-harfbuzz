cmake_minimum_required(VERSION 3.10)

project(drawtext-gl-freetype-harfbuzz)

set(CMAKE_CXX_STANDARD 11)

# Compile GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
add_subdirectory(deps/glfw)

# Compile FreeType2
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE CACHE BOOL " " FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE CACHE BOOL " " FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE CACHE BOOL " " FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE CACHE BOOL " " FORCE)
set(CMAKE_DISABLE_FIND_PACKAGE_BrotliDec TRUE CACHE BOOL " " FORCE)
add_subdirectory(deps/freetype)

# Compile HarfBuzz
set(HB_BUILD_SUBSET OFF CACHE BOOL " " FORCE)
add_subdirectory(deps/harfbuzz)

# drawtext
add_executable(drawtext 
    main.cpp
    scope_guard.h
    deps/glad/src/glad.c
    )

target_include_directories(drawtext
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glm"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/harfbuzz/src")

target_link_libraries(drawtext
    glfw
    freetype
    harfbuzz)