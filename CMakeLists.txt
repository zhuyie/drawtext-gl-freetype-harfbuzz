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
# harfbuzz's CMakeLists.txt use FindFreetype cmake module. don't know how to use the inclusion freetype.
# set(HB_HAVE_FREETYPE ON CACHE BOOL " " FORCE)
# set(ENV{FREETYPE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype")
add_subdirectory(deps/harfbuzz)

# drawtext
add_executable(drawtext 
    main.cpp
    scope_guard.h
    font.h
    font.cpp
    skyline_binpack.h
    skyline_binpack.cpp
    texture_atlas.h
    texture_atlas.cpp
    shader.h
    shader.cpp
    text_render.h
    text_render.cpp
    deps/glad/src/glad.c)

# Workaround for HarfBuzz FreeType integration
target_sources(drawtext PUBLIC deps/harfbuzz/src/hb-ft.cc)
set_source_files_properties(deps/harfbuzz/src/hb-ft.cc
    PROPERTIES COMPILE_DEFINITIONS HAVE_FREETYPE)

# https://docs.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8?view=msvc-160
if (MSVC)
    target_compile_options(drawtext PRIVATE /source-charset:utf-8)
endif()

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