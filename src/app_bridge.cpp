#include "epochgui_demo/app_bridge.hpp"

#include <new>
#include <string>
#include <string_view>

import epoch.gui.demo.opengl;

struct epoch_gui_demo_renderer
{
    epochengine::gui_demo::OpenGLRenderer renderer{};
    std::string copied_text{};
};

const char* epoch_gui_demo_edit_command(epoch_gui_demo_renderer* renderer,
    int command, const char* pasted_utf8)
{
    if (!renderer) return "";
    renderer->copied_text = renderer->renderer.edit_command(command,
        pasted_utf8 ? std::string_view{pasted_utf8} : std::string_view{});
    return renderer->copied_text.c_str();
}

void epoch_gui_demo_set_clipboard(epoch_gui_demo_renderer* renderer,
    const char* (*reader)(void*), bool (*writer)(void*, const char*), void* user)
{
    if (renderer) renderer->renderer.set_clipboard(reader, writer, user);
}

namespace
{
    [[nodiscard]] epochengine::gui_demo::PointerButton pointer_button_from(int value) noexcept
    {
        using Button = epochengine::gui_demo::PointerButton;
        switch (value)
        {
        case EPOCH_GUI_DEMO_POINTER_RIGHT: return Button::right;
        case EPOCH_GUI_DEMO_POINTER_MIDDLE: return Button::middle;
        case EPOCH_GUI_DEMO_POINTER_AUXILIARY_1: return Button::auxiliary_1;
        case EPOCH_GUI_DEMO_POINTER_AUXILIARY_2: return Button::auxiliary_2;
        case EPOCH_GUI_DEMO_POINTER_LEFT:
        default:
            return Button::left;
        }
    }

    [[nodiscard]] epochengine::gui_demo::Key key_from(int value) noexcept
    {
        using Key = epochengine::gui_demo::Key;
        switch (value)
        {
        case EPOCH_GUI_DEMO_KEY_ENTER: return Key::enter;
        case EPOCH_GUI_DEMO_KEY_TAB: return Key::tab;
        case EPOCH_GUI_DEMO_KEY_SPACE: return Key::space;
        case EPOCH_GUI_DEMO_KEY_BACKSPACE: return Key::backspace;
        case EPOCH_GUI_DEMO_KEY_DELETE: return Key::delete_key;
        case EPOCH_GUI_DEMO_KEY_LEFT: return Key::arrow_left;
        case EPOCH_GUI_DEMO_KEY_RIGHT: return Key::arrow_right;
        case EPOCH_GUI_DEMO_KEY_UP: return Key::arrow_up;
        case EPOCH_GUI_DEMO_KEY_DOWN: return Key::arrow_down;
        case EPOCH_GUI_DEMO_KEY_HOME: return Key::home;
        case EPOCH_GUI_DEMO_KEY_END: return Key::end;
        case EPOCH_GUI_DEMO_KEY_SHIFT: return Key::shift;
        case EPOCH_GUI_DEMO_KEY_CONTROL: return Key::control;
        case EPOCH_GUI_DEMO_KEY_ALT: return Key::alt;
        case EPOCH_GUI_DEMO_KEY_SUPER: return Key::super;
        case EPOCH_GUI_DEMO_KEY_ESCAPE:
        default:
            return Key::escape;
        }
    }
}

epoch_gui_demo_renderer* epoch_gui_demo_create(void)
{
    return new (std::nothrow) epoch_gui_demo_renderer{};
}

void epoch_gui_demo_destroy(epoch_gui_demo_renderer* renderer)
{
    delete renderer;
}

bool epoch_gui_demo_initialize(
    epoch_gui_demo_renderer* renderer,
    epoch_gui_demo_proc_loader loader)
{
    if (!renderer || !loader)
        return false;

    return renderer->renderer.initialize(
        reinterpret_cast<epochengine::gui_demo::OpenGLProcLoader>(loader));
}

void epoch_gui_demo_resize(
    epoch_gui_demo_renderer* renderer,
    int width,
    int height)
{
    if (renderer)
        renderer->renderer.resize(width, height);
}

void epoch_gui_demo_pointer_move(
    epoch_gui_demo_renderer* renderer,
    float x,
    float y)
{
    if (renderer)
        renderer->renderer.pointer_move(x, y);
}

void epoch_gui_demo_pointer_button(
    epoch_gui_demo_renderer* renderer,
    int button,
    bool down)
{
    if (renderer)
        renderer->renderer.pointer_button(pointer_button_from(button), down);
}

void epoch_gui_demo_cancel_input(epoch_gui_demo_renderer* renderer)
{
    if (renderer) renderer->renderer.cancel_input();
}

bool epoch_gui_demo_external_view(const epoch_gui_demo_renderer* renderer, unsigned id, epoch_gui_demo_view_info* output)
{
    if (!renderer || !output) return false;
    epochengine::gui_demo::ExternalViewInfo info;
    if (!renderer->renderer.external_view(id, info)) return false;
    *output = {info.id, info.title.data(), info.width, info.height, info.attached};
    return true;
}

bool epoch_gui_demo_initialize_view(epoch_gui_demo_renderer* renderer, unsigned id, epoch_gui_demo_proc_loader loader)
{ return renderer && renderer->renderer.initialize_view(id, reinterpret_cast<epochengine::gui_demo::OpenGLProcLoader>(loader)); }
void epoch_gui_demo_release_view(epoch_gui_demo_renderer* renderer, unsigned id)
{ if (renderer) renderer->renderer.release_view(id); }
void epoch_gui_demo_release_primary(epoch_gui_demo_renderer* renderer)
{ if (renderer) renderer->renderer.release_primary(); }
void epoch_gui_demo_redock_view(epoch_gui_demo_renderer* renderer, unsigned id)
{ if (renderer) renderer->renderer.redock_view(id); }
void epoch_gui_demo_resize_view(epoch_gui_demo_renderer* renderer, unsigned id, int width, int height, float dpi_scale)
{ if (renderer) renderer->renderer.resize_view(id, width, height, dpi_scale); }
void epoch_gui_demo_set_input_source(epoch_gui_demo_renderer* renderer, unsigned id)
{ if (renderer) renderer->renderer.set_input_source(id); }
void epoch_gui_demo_pointer_move_view(epoch_gui_demo_renderer* renderer, unsigned id, float x, float y)
{ if (renderer) renderer->renderer.pointer_move_view(id, x, y); }
bool epoch_gui_demo_render_view(epoch_gui_demo_renderer* renderer, unsigned id)
{ return renderer && renderer->renderer.render_view(id); }
void epoch_gui_demo_report_view_error(epoch_gui_demo_renderer* renderer, const char* message)
{ if (renderer) renderer->renderer.report_view_error(message ? std::string_view{message} : std::string_view{}); }

void epoch_gui_demo_wheel(
    epoch_gui_demo_renderer* renderer,
    float horizontal,
    float vertical)
{
    if (renderer)
        renderer->renderer.wheel(horizontal, vertical);
}

void epoch_gui_demo_key_event(
    epoch_gui_demo_renderer* renderer,
    int key,
    bool down,
    bool repeated)
{
    if (renderer)
        renderer->renderer.key(key_from(key), down, repeated);
}

void epoch_gui_demo_modifiers(
    epoch_gui_demo_renderer* renderer,
    bool shift,
    bool control,
    bool alt,
    bool super_key)
{
    if (!renderer)
        return;

    renderer->renderer.modifiers({
        .shift = shift,
        .control = control,
        .alt = alt,
        .super = super_key
    });
}

void epoch_gui_demo_text_input(
    epoch_gui_demo_renderer* renderer,
    const char* utf8_text)
{
    if (renderer && utf8_text)
        renderer->renderer.text_input(std::string_view{ utf8_text });
}

int epoch_gui_demo_window_hit_test(
    const epoch_gui_demo_renderer* renderer,
    float x,
    float y)
{
    if (!renderer)
        return EPOCH_GUI_DEMO_WINDOW_CLIENT;

    return static_cast<int>(renderer->renderer.window_chrome_hit_test(x, y));
}

int epoch_gui_demo_take_window_command(epoch_gui_demo_renderer* renderer)
{
    if (!renderer)
        return EPOCH_GUI_DEMO_COMMAND_NONE;

    return static_cast<int>(renderer->renderer.take_window_command());
}

int epoch_gui_demo_take_native_frame_mode(epoch_gui_demo_renderer* renderer)
{
    if (!renderer)
        return EPOCH_GUI_DEMO_FRAME_MODE_NONE;

    return renderer->renderer.take_native_frame_mode();
}

void epoch_gui_demo_render(epoch_gui_demo_renderer* renderer)
{
    if (renderer)
        renderer->renderer.render();
}
