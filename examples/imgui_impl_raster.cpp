// dear imgui: Renderer for OpenGL2 (legacy OpenGL, fixed pipeline)
// This needs to be used along with a Platform Binding (e.g. GLFW, SDL, Win32, custom..)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2020-03-11: raster: Created

#include "imgui.h"
#include "imgui_impl_raster.h"

#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif
#include <algorithm>

static ImGuiImplRasterinfo g_Info;
static ImVec4 g_ClipRect;
static uint32_t g_FontTexture = 0;


struct vec2f_t {
  float x, y;
};

struct vec3f_t {
  float x, y, z;
};

static inline float _orient2d(
    const vec2f_t& a,
    const vec2f_t& b,
    const vec2f_t& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void draw_triangle(
    const vec2f_t& v0,
    const vec2f_t& v1,
    const vec2f_t& v2)
{
    // triangle bounds
    int32_t minx = int32_t(std::min({v0.x, v1.x, v2.x}));
    int32_t maxx = int32_t(std::max({v0.x, v1.x, v2.x}));
    int32_t miny = int32_t(std::min({v0.y, v1.y, v2.y}));
    int32_t maxy = int32_t(std::max({v0.y, v1.y, v2.y}));
    // clip min point to screen
#if 0
    minx = std::max<int32_t>(minx + 0, viewport_.x0);
    miny = std::max<int32_t>(miny + 0, viewport_.y0);
    maxx = std::min<int32_t>(maxx + 1, viewport_.x1);
    maxy = std::min<int32_t>(maxy + 1, viewport_.y1);
#endif
    // the signed triangle area
    const float area = 1.f / ((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y));
    // reject back faces
    if (area <= 0.f)
        return;
    // barycentric x step
    const vec3f_t sx = {
        (v0.y - v1.y) * area, // A01
        (v1.y - v2.y) * area, // A12
        (v2.y - v0.y) * area // A20
    };
    // barycentric y step
    const vec3f_t sy = {
        (v1.x - v0.x) * area, // B01
        (v2.x - v1.x) * area, // B12
        (v0.x - v2.x) * area // B20
    };
    // barycentric start point
    vec2f_t p = { float(minx), float(miny) };
    // barycentric value at start point
    vec3f_t wy = {
        _orient2d(v0, v1, p) * area, // W2 -> A01, B01
        _orient2d(v1, v2, p) * area, // W0 -> A12, B12
        _orient2d(v2, v0, p) * area, // W1 -> A20, B20
    };
    const uint32_t dst_pitch = g_Info.pitch;
    const uint32_t colour = 0xffffff;
    uint32_t* pix = g_Info.pixels + int32_t(p.y) * dst_pitch;
    // rendering loop
    for (int32_t py = miny; py < maxy; py++) {
        vec3f_t wx = wy;
        for (int32_t px = minx; px < maxx; px++) {
            // If p is on or inside all edges, render pixel.
            if (wx.x >= 0 && wx.y >= 0 && wx.z >= 0) {
                pix[px] = colour;
            }
            // X step
            wx = vec3f_t{wx.x + sx.x, wx.y + sx.y, wx.z + sx.z};
        }
        // Y step
        wy = vec3f_t{wy.x + sy.x, wy.y + sy.y, wy.z + sy.z};
        pix += dst_pitch;
    }
}

void plot(float x, float y, uint32_t rgb) {
    const int32_t ix = int32_t(x);
    const int32_t iy = int32_t(y);

    const int32_t cx0 = std::max<int32_t>(0, g_ClipRect.x);
    const int32_t cy0 = std::max<int32_t>(0, g_ClipRect.y);
    const int32_t cx1 = std::min<int32_t>(g_Info.width,  g_ClipRect.z);
    const int32_t cy1 = std::min<int32_t>(g_Info.height, g_ClipRect.w);

    if (ix >= cx0 && iy >= cy0 && ix < cx1 && iy < cy1) {
        uint32_t *p = g_Info.pixels + g_Info.pitch * iy + ix;
        *p = rgb;
    }
}

// Functions
bool ImGui_ImplRaster_Init(const ImGuiImplRasterinfo *info)
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_raster";
    g_Info = *info;
    return true;
}

void ImGui_ImplRaster_Shutdown()
{
    ImGui_ImplRaster_DestroyDeviceObjects();
}

void ImGui_ImplRaster_NewFrame()
{
    if (!g_FontTexture)
    {
        ImGui_ImplRaster_CreateDeviceObjects();
    }
}

static void ImGui_ImplRaster_ResetRenderState(ImDrawData* draw_data, int w, int h)
{
}

static void ImGui_ImplRaster_Draw(const ImDrawVert *vert, const ImDrawIdx *idx, uint32_t count)
{
    for (uint32_t i = 0; i < count; i += 3) {
        const ImDrawVert & v0 = vert[idx[i+0]];
        const ImDrawVert & v1 = vert[idx[i+1]];
        const ImDrawVert & v2 = vert[idx[i+2]];
        plot(v0.pos.x, v0.pos.y, 0xffffff);
        plot(v1.pos.x, v1.pos.y, 0xffffff);
        plot(v2.pos.x, v2.pos.y, 0xffffff);

        draw_triangle(
          vec2f_t{v0.pos.x, v0.pos.y},
          vec2f_t{v1.pos.x, v1.pos.y},
          vec2f_t{v2.pos.x, v2.pos.y});

    }
}

void ImGui_ImplRaster_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }

    // Setup desired GL state
    ImGui_ImplRaster_ResetRenderState(draw_data, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplRaster_ResetRenderState(draw_data, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                g_ClipRect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                g_ClipRect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                g_ClipRect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                g_ClipRect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (g_ClipRect.x < fb_width && g_ClipRect.y < fb_height && g_ClipRect.z >= 0.0f && g_ClipRect.w >= 0.0f)
                {
                    ImGui_ImplRaster_Draw(vtx_buffer, idx_buffer, pcmd->ElemCount);
                }
            }
            idx_buffer += pcmd->ElemCount;
        }
    }
}

bool ImGui_ImplRaster_CreateFontsTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is
    // more likely to be compatible with user's existing shaders. If your ImTextureId represent a
    // higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead
    // to save on GPU memory.
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // give it a dummy pointer for now
    io.Fonts->TexID = ImTextureID(-1);
    return true;
}

void ImGui_ImplRaster_DestroyFontsTexture()
{
    if (g_FontTexture)
    {
        // delete stuff
    }
}

bool ImGui_ImplRaster_CreateDeviceObjects()
{
    return ImGui_ImplRaster_CreateFontsTexture();
}

void ImGui_ImplRaster_DestroyDeviceObjects()
{
    ImGui_ImplRaster_DestroyFontsTexture();
}
