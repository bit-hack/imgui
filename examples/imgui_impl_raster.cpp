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

struct rect_t {
  int32_t x0, y0, x1, y1;
};

struct vec2f_t {

  void operator += (const vec2f_t &p) {
    x += p.x; y += p.y;
  }

  float x, y;
};

bool operator == (const ImVec2 &a, const ImVec2 &b) {
  return a.x == b.x && a.y == b.y;
}

struct vec3f_t {

  void operator += (const vec3f_t &p) {
    x += p.x; y += p.y; z += p.z;
  }

  float x, y, z;
};

struct texture_t {
  uint32_t w, h;
  const uint8_t *tex;
};

static ImGuiImplRasterinfo g_Info;
static ImVec4 g_ClipRect;
static uint32_t g_FontTexture;
static rect_t g_viewport;
static rect_t g_clip;
static texture_t g_font;

static inline float orient2d(
    const vec2f_t& a,
    const vec2f_t& b,
    const vec2f_t& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

static inline uint32_t swizzle(uint32_t x) {
  return ((x >> 16) & 0xff) | ((x << 16) & 0xff0000) | (x & 0xff00);
}

void draw_triangle(
    const vec2f_t& v0,
    const vec2f_t& v1,
    const vec2f_t& v2,
    uint32_t rgb)
{
    // triangle bounds
    int32_t minx = int32_t(std::min({v0.x, v1.x, v2.x - 1}));
    int32_t maxx = int32_t(std::max({v0.x, v1.x, v2.x + 1}));
    int32_t miny = int32_t(std::min({v0.y, v1.y, v2.y - 1}));
    int32_t maxy = int32_t(std::max({v0.y, v1.y, v2.y + 1}));
    // clip min point to screen
    minx = std::max<int32_t>({minx + 0, g_viewport.x0, g_clip.x0});
    miny = std::max<int32_t>({miny + 0, g_viewport.y0, g_clip.y0});
    maxx = std::min<int32_t>({maxx + 1, g_viewport.x1, g_clip.x1});
    maxy = std::min<int32_t>({maxy + 1, g_viewport.y1, g_clip.y1});
    // barycentric x step
    const vec3f_t sx = {
        (v0.y - v1.y),  // A01
        (v1.y - v2.y),  // A12
        (v2.y - v0.y)   // A20
    };
    // barycentric y step
    const vec3f_t sy = {
        (v1.x - v0.x),  // B01
        (v2.x - v1.x),  // B12
        (v0.x - v2.x)   // B20
    };
    // barycentric start point
    vec2f_t p = { float(minx) + .5f, float(miny) + .5f };
    // barycentric value at start point
    vec3f_t wy = {
        orient2d(v0, v1, p), // W2 -> A01, B01
        orient2d(v1, v2, p), // W0 -> A12, B12
        orient2d(v2, v0, p), // W1 -> A20, B20
    };
    const uint32_t dst_pitch = g_Info.pitch;
    const uint32_t colour = rgb;
    uint32_t* pix = g_Info.pixels + miny * dst_pitch;
    // rendering loop
    for (int32_t py = miny; py < maxy; py++) {
        vec3f_t wx = wy;
        for (int32_t px = minx; px < maxx; px++) {
            // If p is on or inside all edges, render pixel
            if (wx.x >= 0 && wx.y >= 0 && wx.z >= 0) {
#if 0
                pix[px] = ((pix[px] >> 2) & 0x3f3f3f) +
                           ((colour >> 1) & 0x7f7f7f) +
                           ((colour >> 2) & 0x3f3f3f);
#else
                pix[px] = colour;
#endif
            }
            // X step
            wx += sx;
        }
        // Y step
        wy += sy;
        pix += dst_pitch;
    }
}

void draw_triangle(
    const vec2f_t& v0,
    const vec2f_t& v1,
    const vec2f_t& v2,
    const vec2f_t& t0,
    const vec2f_t& t1,
    const vec2f_t& t2,
    const texture_t & tex)
{
    const uint8_t *t = (const uint8_t*)tex.tex;
    const uint32_t tmask = (tex.w * tex.h) - 1;

    // triangle bounds
    int32_t minx = int32_t(std::min<float>({v0.x, v1.x, v2.x}));
    int32_t maxx = int32_t(std::max<float>({v0.x, v1.x, v2.x}));
    int32_t miny = int32_t(std::min<float>({v0.y, v1.y, v2.y}));
    int32_t maxy = int32_t(std::max<float>({v0.y, v1.y, v2.y}));
    // clip min point to screen
    minx = std::max<int32_t>({minx + 0, g_viewport.x0, g_clip.x0});
    miny = std::max<int32_t>({miny + 0, g_viewport.y0, g_clip.y0});
    maxx = std::min<int32_t>({maxx + 1, g_viewport.x1, g_clip.x1});
    maxy = std::min<int32_t>({maxy + 1, g_viewport.y1, g_clip.y1});
    // the signed triangle areas
    const float area = 1.f / ((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y));
    // reject back faces
    if (area <= 0.f)
        return;
    // barycentric x step
    const vec3f_t sx = {
        (v0.y - v1.y) * area, // A01
        (v1.y - v2.y) * area, // A12
        (v2.y - v0.y) * area  // A20
    };
    // barycentric y step
    const vec3f_t sy = {
        (v1.x - v0.x) * area, // B01
        (v2.x - v1.x) * area, // B12
        (v0.x - v2.x) * area  // B20
    };
    // barycentric start point
    vec2f_t p = {minx + .5f, miny + .5f};
    // barycentric value at start point
    vec3f_t wy = {
        orient2d(v0, v1, p) * area, // W2 -> A01, B01
        orient2d(v1, v2, p) * area, // W0 -> A12, B12
        orient2d(v2, v0, p) * area, // W1 -> A20, B20
    };
    // texture start point
    vec2f_t ty = {
        (wy.x * t2.x) + (wy.y * t0.x) + (wy.z * t1.x),
        (wy.x * t2.y) + (wy.y * t0.y) + (wy.z * t1.y),
    };
    // texture x step
    const vec2f_t tsx = {
        (sx.x * t2.x) + (sx.y * t0.x) + (sx.z * t1.x),
        (sx.x * t2.y) + (sx.y * t0.y) + (sx.z * t1.y)
    };
    // texture y step
    const vec2f_t tsy = {
        (sy.x * t2.x) + (sy.y * t0.x) + (sy.z * t1.x),
        (sy.x * t2.y) + (sy.y * t0.y) + (sy.z * t1.y)
    };

    uint32_t* pix = g_Info.pixels + miny * g_Info.pitch;
    // rendering loop
    for (int32_t py = miny; py < maxy; py++) {
        vec3f_t wx = wy;
        vec2f_t tx = ty;
        for (int32_t px = minx; px < maxx; px++) {
            // If p is on or inside all edges, render pixel.
            if (wx.x >= 0 && wx.y >= 0 && wx.z >= 0) {
              const uint32_t u = uint32_t(tx.x);
              const uint32_t v = uint32_t(tx.y);
              const uint32_t index = u + (v * tex.w);
              const uint32_t rgb = t[index & tmask];
              if (rgb > 0x7f) {
                pix[px] = 0xffffff;
              }
            }
            // X step
            wx += sx;
            tx += tsx;
        }
        // Y step
        wy += sy;
        pix += g_Info.width;
        ty += tsy;
    }
}

// Functions
bool ImGui_ImplRaster_Init(const ImGuiImplRasterinfo *info)
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_raster";
    g_Info = *info;

    // Grab the viewport
    g_viewport.x0 = 0;
    g_viewport.y0 = 0;
    g_viewport.x1 = info->width - 1;
    g_viewport.y1 = info->height - 1;

    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = false;
    style.AntiAliasedFill = false;
    style.WindowRounding = 0;
    style.FrameRounding = 0;
    style.ScrollbarRounding = 0;
    style.ChildRounding = 0;
    style.GrabRounding = 0;
    style.TabRounding = 0;
    style.PopupRounding = 0;

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

        if (v0.uv == v1.uv) {
          draw_triangle(
            vec2f_t{v0.pos.x, v0.pos.y},
            vec2f_t{v1.pos.x, v1.pos.y},
            vec2f_t{v2.pos.x, v2.pos.y},
            swizzle(v0.col));
        } else {
          draw_triangle(
            vec2f_t{v0.pos.x, v0.pos.y},
            vec2f_t{v1.pos.x, v1.pos.y},
            vec2f_t{v2.pos.x, v2.pos.y},
            vec2f_t{v0.uv.x * g_font.w, v0.uv.y * g_font.h},
            vec2f_t{v1.uv.x * g_font.w, v1.uv.y * g_font.h},
            vec2f_t{v2.uv.x * g_font.w, v2.uv.y * g_font.h},
            g_font);
        }
    }
}

void ImGui_ImplRaster_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
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
                g_clip.x0 = int32_t((pcmd->ClipRect.x - clip_off.x) * clip_scale.x);
                g_clip.y0 = int32_t((pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                g_clip.x1 = int32_t((pcmd->ClipRect.z - clip_off.x) * clip_scale.x) + 1;
                g_clip.y1 = int32_t((pcmd->ClipRect.w - clip_off.y) * clip_scale.y) + 1;

                if (g_clip.x0 < fb_width && g_clip.y0 < fb_height && g_clip.x1 >= 0.0f && g_clip.y1 >= 0.0f)
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
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    g_font.w = width;
    g_font.h = height;
    g_font.tex = (const uint8_t*)pixels;

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
