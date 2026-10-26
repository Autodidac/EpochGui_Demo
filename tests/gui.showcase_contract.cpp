// SPDX-License-Identifier: MIT
// Runs the actual renderer/controller using a recording GL transport, not a native window.
#include "epochgui_demo/app_bridge.hpp"
#include "gui/dock_layout.hpp"
#include <cstddef>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

namespace
{
    using U = unsigned;
    int width = 1280, height = 820;
    bool clipped{}, invalid_clip{};
    unsigned clipped_draws{}, draws{};
    struct Point { float x, y; bool operator==(const Point&) const = default; };
    std::vector<Point> uploaded{};
    std::array<float, 4> color{};
    bool edit_label_sample{}, fractional_glyph{};
    unsigned sampled_glyph_pixels{};
    bool probe_enabled{};
    Point probe{};
    std::array<float, 4> probe_color{};
    bool triangle_contains(Point p, Point a, Point b, Point c)
    {
        const auto cross = [](Point p, Point a, Point b) {
            return (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
        };
        if (std::abs(cross(a, b, c)) < 0.00001f) return false;
        const float x = cross(p, a, b), y = cross(p, b, c), z = cross(p, c, a);
        return !((x < 0 || y < 0 || z < 0) && (x > 0 || y > 0 || z > 0));
    }
    unsigned current_context{}, next_object{1};
    bool wrong_context{};
    std::unordered_map<unsigned, unsigned> object_context;
    void generate(int count, U* values)
    {
        for (int n = 0; n < count; ++n)
        { values[n] = next_object++; object_context[values[n]] = current_context; }
    }
    void verify_object(U value)
    {
        if (value && (!object_context.contains(value) || object_context[value] != current_context)) wrong_context = true;
    }
    void delete_objects(int count, const U* values)
    { for (int n = 0; n < count; ++n) { verify_object(values[n]); object_context.erase(values[n]); } }
    void bind_array(U value) { verify_object(value); }
    void bind_buffer(U, U value) { verify_object(value); }
    void one(U) {}
    void two(U, U) {}
    void data(U, std::ptrdiff_t bytes, const void* source, U)
    {
        if (!source || bytes <= 0) { uploaded.clear(); return; }
        const auto* points = static_cast<const Point*>(source);
        uploaded.assign(points, points + bytes / sizeof(Point));
    }
    void attribute(U, int, U, unsigned char, int, const void*) {}
    U shader(U) { return 1; }
    U program() { return 1; }
    void shader_source(U, int, const char* const*, const int*) {}
    void status(U, U, int* success) { *success = 1; }
    void attribute_location(U, U, const char*) {}
    int uniform_location(U, const char*) { return 0; }
    void uniform2(int, float, float) {}
    void uniform4(int, float r, float g, float b, float a) { color = {r, g, b, a}; }
    void clear_color(float, float, float, float) {}
    void viewport(int, int, int w, int h) { width = w; height = h; }
    void enable(U) { clipped = true; }
    void disable(U) { clipped = false; }
    void scissor(int x, int y, int w, int h)
    {
        if (w < 0 || h < 0 || (w > 0 && h > 0
            && (x < 0 || y < 0 || x + w > width || y + h > height))) invalid_clip = true;
    }
    void draw(U, int first, int count)
    {
        ++draws;
        if (clipped) ++clipped_draws;
        if (probe_enabled && !clipped)
            for (int n = first; n + 2 < first + count; n += 3)
                if (triangle_contains(probe, uploaded[n], uploaded[n + 1], uploaded[n + 2]))
                    probe_color = color;
        if (count != 6 || first < 0 || static_cast<std::size_t>(first + count) > uploaded.size()) return;
        if (std::abs(color[0] - 234.0f / 255.0f) > 0.001f
            || std::abs(color[1] - 240.0f / 255.0f) > 0.001f) return;
        Point low = uploaded[first], high = low;
        for (int n = first; n < first + count; ++n)
        {
            low.x = (std::min)(low.x, uploaded[n].x); low.y = (std::min)(low.y, uploaded[n].y);
            high.x = (std::max)(high.x, uploaded[n].x); high.y = (std::max)(high.y, uploaded[n].y);
        }
        // Actual CONTROLS label and SELECT ALL action, not a lookalike font fixture.
        if (edit_label_sample ? (low.x < 281 || high.x > 430 || low.y < 574 || high.y > 586)
                              : (low.x < 25 || high.x > 85 || low.y < 194 || high.y > 205)) return;
        ++sampled_glyph_pixels;
        const float left = low.x * width / 1280.0f, right = high.x * width / 1280.0f;
        const float top = low.y * height / 820.0f, bottom = high.y * height / 820.0f;
        for (const float edge : {left, right, top, bottom})
            if (std::abs(edge - std::round(edge)) > 0.002f) fractional_glyph = true;
        if (right - left < 0.998f || bottom - top < 0.998f) fractional_glyph = true;
    }
    epoch_gui_demo_gl_proc load(const char* name)
    {
#define PROC(n, f) if (std::strcmp(name, n) == 0) return reinterpret_cast<epoch_gui_demo_gl_proc>(&f)
        PROC("glGenVertexArrays", generate); PROC("glGenBuffers", generate);
        PROC("glDeleteVertexArrays", delete_objects); PROC("glDeleteBuffers", delete_objects);
        PROC("glBindVertexArray", bind_array); PROC("glBindBuffer", bind_buffer); PROC("glBufferData", data);
        PROC("glEnableVertexAttribArray", one); PROC("glVertexAttribPointer", attribute);
        PROC("glCreateShader", shader); PROC("glShaderSource", shader_source); PROC("glCompileShader", one);
        PROC("glGetShaderiv", status); PROC("glDeleteShader", one); PROC("glCreateProgram", program);
        PROC("glAttachShader", two); PROC("glBindAttribLocation", attribute_location);
        PROC("glLinkProgram", one); PROC("glGetProgramiv", status); PROC("glDeleteProgram", one);
        PROC("glUseProgram", one); PROC("glGetUniformLocation", uniform_location);
        PROC("glUniform2f", uniform2); PROC("glUniform4f", uniform4); PROC("glDrawArrays", draw);
        PROC("glClearColor", clear_color); PROC("glClear", one); PROC("glViewport", viewport);
        PROC("glEnable", enable); PROC("glDisable", disable); PROC("glScissor", scissor);
#undef PROC
        return nullptr;
    }
    void click(epoch_gui_demo_renderer* renderer, float x, float y)
    {
        epoch_gui_demo_pointer_move(renderer, x * width / 1280.0f, y * height / 820.0f);
        epoch_gui_demo_pointer_button(renderer, EPOCH_GUI_DEMO_POINTER_LEFT, true);
        epoch_gui_demo_render(renderer);
        epoch_gui_demo_pointer_button(renderer, EPOCH_GUI_DEMO_POINTER_LEFT, false);
        epoch_gui_demo_render(renderer);
    }
    bool refused_clipboard(void*, const char*) { return false; }
}

namespace
{
    int window_input_regressions()
    {
        for (int scenario = 0; scenario < 13; ++scenario)
        {
            auto* renderer = epoch_gui_demo_create();
            if (!renderer || !epoch_gui_demo_initialize(renderer, load)) return 30;
            struct Cleanup { epoch_gui_demo_renderer* value; ~Cleanup() { epoch_gui_demo_destroy(value); } } cleanup{renderer};
            epoch_gui_demo_resize(renderer, 1280, 820);
            epoch_gui_demo_render(renderer);
            click(renderer, 250.0f, 80.0f);
            const auto move = [&](float x, float y) { epoch_gui_demo_pointer_move(renderer, x, y); };
            const auto button = [&](bool down) { epoch_gui_demo_pointer_button(renderer, EPOCH_GUI_DEMO_POINTER_LEFT, down); };
            if (scenario == 0)
            {
                click(renderer, 688.0f, 266.0f);
                click(renderer, 1078.0f, 376.0f);
                const auto before = uploaded;
                move(650.0f, 760.0f); button(true); epoch_gui_demo_render(renderer);
                move(350.0f, 228.0f); button(false); epoch_gui_demo_render(renderer);
                // A release over a dock title cannot act without its matching title press.
                if (uploaded != before) return 31;
            }
            else if (scenario == 1)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                move(290.0f, 180.0f); epoch_gui_demo_render(renderer);
                button(false); epoch_gui_demo_render(renderer);
                click(renderer, 590.0f, 180.0f); // Close the moved/clamped core floater.
                probe_enabled = true; probe = {530.0f, 180.0f}; epoch_gui_demo_render(renderer);
                // Dragging across RECT must leave BOTH selected beneath the captured floater.
                if (std::abs(probe_color[0] - 49.0f / 255.0f) > 0.001f) return 32;
            }
            else if (scenario == 2)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                move(458.0f, 720.0f); epoch_gui_demo_render(renderer);
                button(false); epoch_gui_demo_render(renderer);
                probe_enabled = true; probe = {960.0f, 650.0f}; epoch_gui_demo_render(renderer);
                // The drag's release cannot activate the unrelated FLOAT action under it.
                if (std::abs(probe_color[0] - 15.0f / 255.0f) > 0.001f) return 33;
            }
            else if (scenario == 3)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                epoch_gui_demo_cancel_input(renderer);
                epoch_gui_demo_render(renderer);
                const auto before = uploaded;
                move(350.0f, 228.0f); button(false); epoch_gui_demo_render(renderer);
                // Focus/capture cancellation must never manufacture a pane click on release.
                if (uploaded != before) return 34;
            }
            else if (scenario == 4)
            {
                click(renderer, 80.0f, 250.0f);
                click(renderer, 400.0f, 400.0f);
                epoch_gui_demo_text_input(renderer, "CANCELLED INPUT MUST NOT BE INSERTED");
                epoch_gui_demo_cancel_input(renderer);
                epoch_gui_demo_render(renderer);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                const std::string copied = epoch_gui_demo_edit_command(renderer, 1, nullptr);
                if (copied.find("Write a note here") == std::string::npos
                    || copied.find("CANCELLED INPUT") != std::string::npos) return 35;
            }
            else if (scenario == 5)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                move(1250.0f, 30.0f); epoch_gui_demo_render(renderer);
                button(false); epoch_gui_demo_render(renderer);
                if (epoch_gui_demo_take_window_command(renderer) != EPOCH_GUI_DEMO_COMMAND_NONE) return 36;
            }
            else if (scenario == 6)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                move(1100.0f, 80.0f); epoch_gui_demo_render(renderer);
                button(false); epoch_gui_demo_render(renderer);
                if (epoch_gui_demo_take_native_frame_mode(renderer) != EPOCH_GUI_DEMO_FRAME_MODE_NONE) return 37;
            }
            else if (scenario == 7)
            {
                const auto before = uploaded;
                move(650.0f, 760.0f); button(true); epoch_gui_demo_render(renderer);
                move(430.0f, 80.0f); button(false); epoch_gui_demo_render(renderer);
                if (uploaded != before) return 38;
            }
            else if (scenario == 8)
            {
                // A full gesture can arrive before the host's next render callback.
                move(480.0f, 266.0f); button(true);
                move(600.0f, 400.0f); button(false);
                epochengine::gui_lib::FloatingWindowState expected{.position = {320.0f, 250.0f},
                    .size = {390.0f, 250.0f}, .open = true, .initialized = true};
                const epochengine::gui_lib::FloatingWindowOptions options{.default_position = expected.position,
                    .default_size = expected.size, .min_size = {270.0f, 190.0f},
                    .viewport_size = {1280.0f, 820.0f}, .title_bar_height = 32.0f, .content_padding = 10.0f};
                (void)epochengine::gui_lib::update_floating_window(expected, options,
                    {.mouse_position = {480.0f, 266.0f}, .mouse_down = true, .mouse_pressed = true});
                const auto moved = epochengine::gui_lib::update_floating_window(expected, options,
                    {.mouse_position = {600.0f, 400.0f}, .mouse_down = true});
                probe_enabled = true;
                probe = {moved.title_bar.position.x + moved.title_bar.size.x * 0.75f,
                    moved.title_bar.position.y + moved.title_bar.size.y - 4.0f};
                epoch_gui_demo_render(renderer);
                if (std::abs(probe_color[0] - 49.0f / 255.0f) > 0.001f) return 39;
            }
            else if (scenario == 9)
            {
                move(480.0f, 266.0f); button(true); epoch_gui_demo_render(renderer);
                const auto before = uploaded;
                for (int index = 0; index < 300; ++index)
                    epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_SPACE, (index % 2) == 0, false);
                epoch_gui_demo_render(renderer);
                move(350.0f, 228.0f); button(false); epoch_gui_demo_render(renderer);
                if (uploaded != before) return 40;
            }
            else if (scenario == 10)
            {
                epoch_gui_demo_modifiers(renderer, true, true, false, false);
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, true, false);
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, false, false);
                epoch_gui_demo_modifiers(renderer, false, false, false, false);
                epoch_gui_demo_render(renderer); // Ctrl+Shift+Tab goes backward to Widgets, not Libraries.
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, true, false);
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, false, false);
                epoch_gui_demo_render(renderer);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != "A SMALL PROJECT") return 41;
            }
            else if (scenario == 11)
            {
                click(renderer, 60.0f, 250.0f);
                click(renderer, 300.0f, 270.0f);
                epoch_gui_demo_text_input(renderer, "FIRST");
                move(400.0f, 400.0f); button(true); button(false);
                epoch_gui_demo_text_input(renderer, "SECOND");
                epoch_gui_demo_render(renderer);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                const std::string notes = epoch_gui_demo_edit_command(renderer, 1, nullptr);
                click(renderer, 300.0f, 270.0f);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                const std::string name = epoch_gui_demo_edit_command(renderer, 1, nullptr);
                if (notes.find("SECOND") == std::string::npos || notes.find("FIRST") != std::string::npos
                    || name.find("FIRST") == std::string::npos || name.find("SECOND") != std::string::npos) return 42;
            }
            else
            {
                click(renderer, 60.0f, 250.0f);
                click(renderer, 300.0f, 270.0f);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                epoch_gui_demo_text_input(renderer, "ABC");
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_BACKSPACE, true, false);
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_BACKSPACE, true, true);
                epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_BACKSPACE, false, false);
                epoch_gui_demo_render(renderer);
                (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
                if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != "A") return 43;
            }
        }
        return 0;
    }
}

int main()
{
    auto* renderer = epoch_gui_demo_create();
    if (!renderer || !epoch_gui_demo_initialize(renderer, load)) return 1;
    struct Cleanup { epoch_gui_demo_renderer* value; ~Cleanup() { epoch_gui_demo_destroy(value); } } cleanup{renderer};
    for (const auto size : {std::array{1280, 820}, std::array{1000, 650}, std::array{1973, 1207}})
    {
        sampled_glyph_pixels = 0;
        fractional_glyph = false;
        epoch_gui_demo_resize(renderer, size[0], size[1]);
        epoch_gui_demo_render(renderer);
        if (fractional_glyph || sampled_glyph_pixels < 30) return 8;
    }
    click(renderer, 60.0f, 250.0f); // Text jump scrolls the shared showcase.
    click(renderer, 300.0f, 400.0f); // Now inside the multiline control after transform.
    (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
    const std::string original = epoch_gui_demo_edit_command(renderer, 1, nullptr);
    if (original.find("Write a note here") == std::string::npos) return 2;
    edit_label_sample = true;
    sampled_glyph_pixels = 0;
    fractional_glyph = false;
    epoch_gui_demo_render(renderer);
    if (fractional_glyph || sampled_glyph_pixels < 30) return 9;
    const std::string long_text = std::string(12000, 'x') + "\nLAST LINE";
    (void)epoch_gui_demo_edit_command(renderer, 3, long_text.c_str());
    epoch_gui_demo_render(renderer);
    (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
    if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != long_text) return 3;
    epoch_gui_demo_set_clipboard(renderer, nullptr, refused_clipboard, nullptr);
    (void)epoch_gui_demo_edit_command(renderer, 2, nullptr);
    epoch_gui_demo_set_clipboard(renderer, nullptr, nullptr, nullptr);
    if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != long_text) return 4;
    epoch_gui_demo_wheel(renderer, 0.0f, 100.0f);
    epoch_gui_demo_render(renderer);
    epoch_gui_demo_resize(renderer, 1000, 650);
    epoch_gui_demo_render(renderer);
    click(renderer, 300.0f, 270.0f); // Single-line field with resized hit mapping.
    (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
    if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != "A SMALL PROJECT") return 5;
    epoch_gui_demo_modifiers(renderer, false, true, false, false);
    epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, true, false);
    epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, false, false);
    epoch_gui_demo_modifiers(renderer, false, false, false, false);
    epoch_gui_demo_render(renderer);
    if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != "") return 6;
    probe_enabled = true;
    probe = {800.0f, 650.0f}; // Visible hosted-window body, previously never drawn.
    epoch_gui_demo_render(renderer);
    if (std::abs(probe_color[0] - 20.0f / 255.0f) > 0.001f) return 10;
    probe = {290.0f, 180.0f}; // Visible RECT control, not the old hit rectangle 32px above it.
    click(renderer, probe.x, probe.y);
    if (std::abs(probe_color[0] - 49.0f / 255.0f) > 0.001f) return 11;
    click(renderer, 688.0f, 266.0f); // Close the separate core floater before testing pane stacking.
    // Compute the title/cascade through the real Gui layout API, not mirrored positioning math.
    namespace gui = epochengine::gui_lib;
    gui::DockLayoutState expected_state;
    gui::DockPaneState expected_tree{.id = 1, .slot = gui::DockSlot::left,
        .min_size = {120.0f, 90.0f}, .weight = 1.0f, .visible = true};
    const gui::DockLayoutOptions expected_options{.workspace = {{246.0f, 214.0f}, {970.0f, 176.0f}},
        .min_pane_size = {96.0f, 64.0f}, .default_popout_size = {300.0f, 190.0f},
        .edge_fraction = 0.22f, .title_bar_height = 28.0f, .content_padding = 6.0f,
        .popout_spacing = 22.0f, .visible_margin = 48.0f};
    gui::normalize_dock_layout(expected_state, &expected_tree, 1, expected_options);
    const auto dock_title = gui::make_dock_pane_layout(expected_tree, expected_options, {}).title_bar;
    expected_tree.popped_out = true;
    const auto popout_title = gui::make_dock_pane_layout(expected_tree, expected_options, {}).title_bar;
    probe = {popout_title.position.x + popout_title.size.x * 0.75f,
             popout_title.position.y + popout_title.size.y * 0.5f};
    const auto docked_geometry = uploaded;
    click(renderer, dock_title.position.x + dock_title.size.x * 0.5f,
        dock_title.position.y + dock_title.size.y * 0.5f);
    if (uploaded == docked_geometry) return 12;
    if (std::abs(probe_color[0] - 27.0f / 255.0f) > 0.001f) return 13;
    // Exercise the real secondary-surface API and require per-context object ownership.
    epoch_gui_demo_view_info view{};
    if (epoch_gui_demo_external_view(renderer, 0, &view)
        || epoch_gui_demo_external_view(renderer, 5, &view)
        || !epoch_gui_demo_external_view(renderer, 1, &view) || view.attached) return 44;
    const auto primary_objects = object_context.size();
    current_context = 1;
    if (!epoch_gui_demo_initialize_view(renderer, 1, load)
        || !epoch_gui_demo_external_view(renderer, 1, &view) || !view.attached) return 45;
    epoch_gui_demo_resize_view(renderer, 1, 600, 380, 2.0f);
    epoch_gui_demo_pointer_move_view(renderer, 1, 120.0f, 100.0f);
    if (!epoch_gui_demo_render_view(renderer, 1) || width != 600 || height != 380) return 46;
    current_context = 0;
    epoch_gui_demo_render(renderer);
    current_context = 1;
    epoch_gui_demo_redock_view(renderer, 1);
    if (epoch_gui_demo_external_view(renderer, 1, &view) || epoch_gui_demo_render_view(renderer, 1)) return 47;
    epoch_gui_demo_release_view(renderer, 1);
    if (object_context.size() != primary_objects) return 48;
    current_context = 0;
    epoch_gui_demo_render(renderer);
    if (wrong_context) return 49;
    if (invalid_clip || clipped || clipped_draws == 0 || draws < clipped_draws) return 7;
    return window_input_regressions();
}
