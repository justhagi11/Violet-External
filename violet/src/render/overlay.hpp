#pragma once
#include <string>
#include <chrono>
#include "../math/math.hpp"
#include "../deps/imgui/imgui.h"
#include "../deps/imgui/imgui_internal.h"
#include "../mesh/parse.hpp"

namespace overlay {
    bool initialize();
    void render_loop();
    void shutdown();
}

namespace render {
    void draw_chams_mesh(const std::vector<mesh_parser::vertex_t>& vertices, const std::vector<mesh_parser::face_t>& faces, const ViewMatrix& vm, const CFrame& cframe, const Vec3& scale, ImU32 color, bool wireframe);
    void flush_chams_meshes();

    inline ImDrawList* get_draw_list() {
        return ImGui::GetBackgroundDrawList();
    }

    inline ImU32 to_color(int r, int g, int b, int a = 255) {
        return IM_COL32(r, g, b, a);
    }

    inline ImU32 get_rainbow_color(float speed = 1.0f, float alpha = 1.0f) {
        static float time = 0.0f;
        time += ImGui::GetIO().DeltaTime * speed;
        
        float r = (sinf(time) * 0.5f + 0.5f);
        float g = (sinf(time + PI_2 / 3.0f) * 0.5f + 0.5f);
        float b = (sinf(time + 2.0f * PI_2 / 3.0f) * 0.5f + 0.5f);
        
        return to_color((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), (int)(alpha * 255.0f));
    }


    inline void text(Vector2 pos, const std::string& text, ImU32 color, bool centered = false, float scale = 1.0f) {
        float original_scale = ImGui::GetIO().FontGlobalScale;
        ImGui::GetIO().FontGlobalScale = scale;
        
        ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
        ImVec2 render_pos = ImVec2(pos.x, pos.y);

        if (centered) {
            render_pos.x -= text_size.x / 2.0f;
            render_pos.y -= text_size.y / 2.0f;
        }

        uint32_t alpha = (color >> IM_COL32_A_SHIFT) & 0xFF;
        get_draw_list()->AddText(ImVec2(render_pos.x + 1, render_pos.y + 1), IM_COL32(0, 0, 0, (int)(200 * (alpha / 255.0f))), text.c_str());
        get_draw_list()->AddText(render_pos, color, text.c_str());

        ImGui::GetIO().FontGlobalScale = original_scale;
    }

    inline void line(Vector2 start, Vector2 end, ImU32 color, float thickness = 1.0f) {
        get_draw_list()->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, end.y), color, thickness);
    }

    inline void rect_filled(Vector2 pos, Vector2 size, ImU32 color, float rounding = 0.0f) {
        get_draw_list()->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), color, rounding);
    }

    inline void rect(Vector2 pos, Vector2 size, ImU32 color, float rounding = 0.0f, float thickness = 1.0f) {
        get_draw_list()->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), color, rounding, 0, thickness);
    }

    inline void gradient_multi(Vector2 pos, Vector2 size, ImU32 top_left, ImU32 top_right, ImU32 bottom_right, ImU32 bottom_left) {
        get_draw_list()->AddRectFilledMultiColor(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + size.x, pos.y + size.y),
            top_left, top_right, bottom_right, bottom_left
        );
    }

    inline void AddRadialGradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out)
    {

        if (((col_in | col_out) & IM_COL32_A_MASK) == 0 || radius < 0.5f)
            return;

        draw_list->_PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
        const int count = draw_list->_Path.Size - 1;

        unsigned int vtx_base = draw_list->_VtxCurrentIdx;
        draw_list->PrimReserve(count * 3, count + 1);

        const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
        draw_list->PrimWriteVtx(center, uv, col_in);
        for (int n = 0; n < count; n++)
            draw_list->PrimWriteVtx(draw_list->_Path[n], uv, col_out);

        for (int n = 0; n < count; n++)
        {
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
            draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
        }
        draw_list->_Path.Size = 0;

    }

    inline float get_real_time() {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(current_time - start_time).count();
    }

    struct Notification {
        std::string text;
        float spawn_time;
        float duration;
    };

    inline std::vector<Notification>& get_notifications() {
        static std::vector<Notification> inst;
        return inst;
    }

    inline void add_notification(const std::string& text, float duration = 3.0f) {
        get_notifications().push_back({ text, get_real_time(), duration });
    }

    void render_notifications();

    inline void gradient_vertical(Vector2 pos, Vector2 size, ImU32 top_color, ImU32 bottom_color) {
        gradient_multi(pos, size, top_color, top_color, bottom_color, bottom_color);
    }

    inline void gradient_horizontal(Vector2 pos, Vector2 size, ImU32 left_color, ImU32 right_color) {
        gradient_multi(pos, size, left_color, right_color, right_color, left_color);
    }

    inline void circle(Vector2 center, float radius, ImU32 color, int segments = 72, float thickness = 1.0f) {
        get_draw_list()->AddCircle(ImVec2(center.x, center.y), radius, color, segments, thickness);
    }

    inline void circle_filled(Vector2 center, float radius, ImU32 color, int segments = 72) {
        get_draw_list()->AddCircleFilled(ImVec2(center.x, center.y), radius, color, segments);
    }

    inline void fov_circle(Vector2 center, float radius, ImU32 color, float thickness = 1.0f) {
        circle(center, radius, color, 128, thickness);
    }
}
