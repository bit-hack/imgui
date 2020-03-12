cmake_minimum_required(VERSION 2.8)
project(my_imgui)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

include_directories("libs/imgui")
set(IMGUI_SRC
    libs/imgui/imconfig.h
    libs/imgui/imgui.cpp
    libs/imgui/imgui.h
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_internal.h
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imstb_rectpack.h
    libs/imgui/imstb_textedit.h
    libs/imgui/imstb_truetype.h

    libs/imgui/imgui_demo.cpp

    libs/imgui/examples/imgui_impl_sdl.cpp
    libs/imgui/examples/imgui_impl_sdl.h

    libs/imgui/examples/imgui_impl_opengl2.cpp
    libs/imgui/examples/imgui_impl_opengl2.h

    libs/imgui/examples/imgui_impl_raster.cpp
    libs/imgui/examples/imgui_impl_raster.h
    )
add_library(IMGUI ${IMGUI_SRC})

file(GLOB SOURCE source/*.cpp source/*.h)
add_executable(my_imgui ${SOURCE})
target_link_libraries(my_imgui ${SDL2_LIBRARIES} IMGUI OPENGL32)
