// dear imgui: Renderer for Software Rasterization
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once
#include <cstdint>

struct ImGuiImplRasterinfo {
  uint32_t *pixels;
  uint32_t width, height;
  uint32_t pitch;
};

IMGUI_IMPL_API bool     ImGui_ImplRaster_Init(const ImGuiImplRasterinfo *info);
IMGUI_IMPL_API void     ImGui_ImplRaster_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplRaster_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplRaster_RenderDrawData(ImDrawData* draw_data);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool     ImGui_ImplRaster_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplRaster_DestroyFontsTexture();
IMGUI_IMPL_API bool     ImGui_ImplRaster_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplRaster_DestroyDeviceObjects();
