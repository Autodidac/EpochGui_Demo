// SPDX-License-Identifier: MIT
// Runs the actual renderer/controller using a recording GL transport, not a native window.
#include "epochgui_demo/app_bridge.hpp"
#include <cstddef>
#include <cstring>
#include <string>

namespace
{
    using U = unsigned;
    int width = 1280, height = 820;
    bool clipped{}, invalid_clip{};
    unsigned clipped_draws{}, draws{};
    void generate(int count, U* values) { for (int n = 0; n < count; ++n) values[n] = U(n + 1); }
    void delete_objects(int, const U*) {}
    void one(U) {}
    void two(U, U) {}
    void data(U, std::ptrdiff_t, const void*, U) {}
    void attribute(U, int, U, unsigned char, int, const void*) {}
    U shader(U) { return 1; }
    U program() { return 1; }
    void shader_source(U, int, const char* const*, const int*) {}
    void status(U, U, int* success) { *success = 1; }
    void attribute_location(U, U, const char*) {}
    int uniform_location(U, const char*) { return 0; }
    void uniform2(int, float, float) {}
    void uniform4(int, float, float, float, float) {}
    void clear_color(float, float, float, float) {}
    void viewport(int, int, int w, int h) { width = w; height = h; }
    void enable(U) { clipped = true; }
    void disable(U) { clipped = false; }
    void scissor(int x, int y, int w, int h)
    {
        if (w < 0 || h < 0 || (w > 0 && h > 0
            && (x < 0 || y < 0 || x + w > width || y + h > height))) invalid_clip = true;
    }
    void draw(U, int, int) { ++draws; if (clipped) ++clipped_draws; }
    epoch_gui_demo_gl_proc load(const char* name)
    {
#define PROC(n, f) if (std::strcmp(name, n) == 0) return reinterpret_cast<epoch_gui_demo_gl_proc>(&f)
        PROC("glGenVertexArrays", generate); PROC("glGenBuffers", generate);
        PROC("glDeleteVertexArrays", delete_objects); PROC("glDeleteBuffers", delete_objects);
        PROC("glBindVertexArray", one); PROC("glBindBuffer", two); PROC("glBufferData", data);
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

int main()
{
    auto* renderer = epoch_gui_demo_create();
    if (!renderer || !epoch_gui_demo_initialize(renderer, load)) return 1;
    struct Cleanup { epoch_gui_demo_renderer* value; ~Cleanup() { epoch_gui_demo_destroy(value); } } cleanup{renderer};
    epoch_gui_demo_resize(renderer, width, height);
    epoch_gui_demo_render(renderer);
    click(renderer, 60.0f, 250.0f); // Text jump scrolls the shared showcase.
    click(renderer, 300.0f, 400.0f); // Now inside the multiline control after transform.
    (void)epoch_gui_demo_edit_command(renderer, 0, nullptr);
    const std::string original = epoch_gui_demo_edit_command(renderer, 1, nullptr);
    if (original.find("Write a note here") == std::string::npos) return 2;
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
    epoch_gui_demo_render(renderer);
    epoch_gui_demo_key_event(renderer, EPOCH_GUI_DEMO_KEY_TAB, false, false);
    epoch_gui_demo_render(renderer);
    if (std::string(epoch_gui_demo_edit_command(renderer, 1, nullptr)) != "") return 6;
    if (invalid_clip || clipped || clipped_draws == 0 || draws < clipped_draws) return 7;
    return 0;
}
