cmake_minimum_required(VERSION 3.0)
project(imgui_examples)

set(OpenGL_GL_PREFERENCE "LEGACY")

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
file(GLOB IMGUI_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/../*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")
add_library(imgui ${IMGUI_SRC})

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
add_executable(
  imgui_SDL2_opengl2
  "imgui_impl_opengl2.cpp"
  "imgui_impl_opengl2.h"
  "imgui_impl_sdl.cpp"
  "imgui_impl_sdl.h"
  "example_sdl_opengl2/main.cpp")
target_link_libraries(imgui_SDL2_opengl2 imgui
  ${SDL2_LIBRARIES}
  ${OPENGL_gl_LIBRARY})

# ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
add_executable(
  imgui_SDL2_raster
  "imgui_impl_raster.cpp"
  "imgui_impl_raster.h"
  "imgui_impl_sdl.cpp"
  "imgui_impl_sdl.h"
  "example_sdl_raster/main.cpp")
target_link_libraries(imgui_SDL2_raster imgui
  ${SDL2_LIBRARIES})

