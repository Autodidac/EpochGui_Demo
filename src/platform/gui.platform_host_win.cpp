// SPDX-License-Identifier: MIT
#include "gui.wgl_helpers.hpp"
#include <epochplatformengine/epoch_platform_engine.hpp>
#include <array>
#include <chrono>
#include <expected>
#include <new>
#include <string>

namespace
{
    namespace host = epochengine::platform;
    class Application final : public host::IApplication
    {
        struct Surface
        {
            host::ContextId context{};
            unsigned view{};
            HWND window{};
            HDC dc{};
            HGLRC gl{};
            wchar_t high_surrogate{};
            unsigned held_buttons{};
            bool closing_by_model{};
            epochgui_demo::win32::QueuedModifiers modifiers{};
        };
        std::array<Surface, 5> surfaces_{};
        epoch_gui_demo_renderer* renderer_{};

        void resize(Surface& surface, int width, int height)
        {
            if (!surface.view) epoch_gui_demo_resize(renderer_, width, height);
            else epoch_gui_demo_resize_view(renderer_, surface.view, width, height,
                static_cast<float>(GetDpiForWindow(surface.window)) / 96.0f);
        }
        void shutdown_graphics(Surface& surface) noexcept
        {
            if (surface.gl && surface.dc) wglMakeCurrent(surface.dc, surface.gl);
            if (renderer_)
            {
                if (surface.view) epoch_gui_demo_release_view(renderer_, surface.view);
                else epoch_gui_demo_release_primary(renderer_);
            }
            wglMakeCurrent(nullptr, nullptr);
            if (surface.gl) wglDeleteContext(surface.gl);
            if (surface.dc && surface.window) ReleaseDC(surface.window, surface.dc);
            surface.gl = nullptr; surface.dc = nullptr; surface.window = nullptr;
            surface.held_buttons = 0;
        }
        void event(Surface& surface, const host::WindowEvent& event)
        {
            if (!renderer_) return;
            epoch_gui_demo_set_input_source(renderer_, surface.view);
            using enum host::WindowEventType;
            switch (event.type)
            {
            case close_requested:
                if (surface.view && !surface.closing_by_model) epoch_gui_demo_redock_view(renderer_, surface.view);
                break;
            case dpi_changed:
            {
                RECT client{};
                if (GetClientRect(surface.window, &client)) resize(surface, client.right, client.bottom);
                break;
            }
            case resized:
                resize(surface, event.width, event.height);
                break;
            case mouse_moved:
                epoch_gui_demo_pointer_move_view(renderer_, surface.view, static_cast<float>(event.x), static_cast<float>(event.y));
                break;
            case mouse_button_pressed:
            case mouse_button_released:
                epoch_gui_demo_pointer_move_view(renderer_, surface.view, static_cast<float>(event.x), static_cast<float>(event.y));
                if (event.button >= 1 && event.button <= 3)
                {
                    const auto bit = 1u << (event.button - 1);
                    if (event.type == mouse_button_pressed) surface.held_buttons |= bit;
                    else surface.held_buttons &= ~bit;
                    epoch_gui_demo_pointer_button(renderer_, event.button - 1, event.type == mouse_button_pressed);
                    if (surface.held_buttons) SetCapture(surface.window);
                    else if (GetCapture() == surface.window) ReleaseCapture();
                }
                break;
            case mouse_wheel:
                epoch_gui_demo_pointer_move_view(renderer_, surface.view, static_cast<float>(event.x), static_cast<float>(event.y));
                epoch_gui_demo_wheel(renderer_, 0.0f, static_cast<float>(event.wheel_delta) / 120.0f);
                break;
            case key_pressed:
            case key_released:
                surface.modifiers.key(event.key, event.type == key_pressed);
                epoch_gui_demo_modifiers(renderer_, surface.modifiers.shift, surface.modifiers.control,
                    surface.modifiers.alt, surface.modifiers.super);
                if (epochgui_demo::win32::dispatch_edit_shortcut_with_modifiers(renderer_, event.key,
                        event.type == key_pressed, event.repeated, surface.modifiers)) break;
                if (const int key = epochgui_demo::win32::key_from_virtual_key(event.key); key >= 0)
                    epoch_gui_demo_key_event(renderer_, key, event.type == key_pressed, event.repeated);
                break;
            case text_input:
            {
                if (event.codepoint < 32 || event.codepoint == 127) break;
                const wchar_t unit = static_cast<wchar_t>(event.codepoint);
                if (unit >= 0xd800 && unit <= 0xdbff) { surface.high_surrogate = unit; break; }
                wchar_t units[2]{unit, 0};
                int count = 1;
                if (surface.high_surrogate && unit >= 0xdc00 && unit <= 0xdfff)
                { units[0] = surface.high_surrogate; units[1] = unit; count = 2; }
                surface.high_surrogate = 0;
                char utf8[9]{};
                const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, units, count, utf8, 8, nullptr, nullptr);
                if (bytes > 0) epoch_gui_demo_text_input(renderer_, utf8);
                break;
            }
            case focus_lost:
                surface.modifiers = {};
                surface.held_buttons = 0;
                if (GetCapture() == surface.window) ReleaseCapture();
                epoch_gui_demo_cancel_input(renderer_);
                surface.high_surrogate = 0;
                break;
            default: break;
            }
        }

        host::Result<host::ContextId> create_surface(host::Runtime& runtime, unsigned view,
            const char* title, int width, int height)
        {
            auto& surface = surfaces_[view];
            surface.view = view;
            surface.closing_by_model = false;
            auto created = runtime.create_context({.driver = host::DriverKind::win32,
                .type = host::ContextType::opengl, .thread_policy = host::ThreadPolicy::caller_thread,
                .window = {.title = title, .width = width, .height = height,
                    .mode = view ? host::WindowMode::detached : host::WindowMode::top_level},
                .route = view ? "gui-demo-popout" : "gui-demo"}, {
                .initialize = [this, view](const host::ContextSnapshot& snapshot) -> host::Status {
                    auto& slot = surfaces_[view];
                    slot.window = static_cast<HWND>(snapshot.native.window);
                    slot.dc = GetDC(slot.window);
                    if (!slot.dc || !epochgui_demo::win32::create_opengl_context(slot.dc, slot.gl))
                        return std::unexpected(host::Error{host::ErrorCode::initialization_failed, "WGL initialization failed"});
                    if (!view) renderer_ = epoch_gui_demo_create();
                    const bool initialized = renderer_ && (view
                        ? epoch_gui_demo_initialize_view(renderer_, view, &epochgui_demo::win32::load_opengl_proc)
                        : epoch_gui_demo_initialize(renderer_, &epochgui_demo::win32::load_opengl_proc));
                    if (!initialized)
                        return std::unexpected(host::Error{host::ErrorCode::initialization_failed, "GUI renderer initialization failed"});
                    if (!view) epochgui_demo::win32::configure_renderer_clipboard(renderer_);
                    RECT client{};
                    GetClientRect(slot.window, &client);
                    resize(slot, client.right, client.bottom);
                    return {};
                },
                .frame = [this, view](const host::ContextSnapshot&, std::chrono::nanoseconds) {
                    auto& slot = surfaces_[view];
                    if (!renderer_ || !slot.gl || !wglMakeCurrent(slot.dc, slot.gl)) return false;
                    if (slot.held_buttons && GetCapture() != slot.window)
                    {
                        slot.held_buttons = 0;
                        epoch_gui_demo_cancel_input(renderer_);
                    }
                    if (view)
                    {
                        if (!epoch_gui_demo_render_view(renderer_, view)) return false;
                    }
                    else epoch_gui_demo_render(renderer_);
                    SwapBuffers(slot.dc);
                    if (view) return true;
                    const int command = epoch_gui_demo_take_window_command(renderer_);
                    if (command == EPOCH_GUI_DEMO_COMMAND_CLOSE) return false;
                    if (command == EPOCH_GUI_DEMO_COMMAND_MINIMIZE) ShowWindow(slot.window, SW_MINIMIZE);
                    if (command == EPOCH_GUI_DEMO_COMMAND_TOGGLE_MAXIMIZE)
                        ShowWindow(slot.window, IsZoomed(slot.window) ? SW_RESTORE : SW_MAXIMIZE);
                    return true;
                },
                .shutdown = [this, view](const host::ContextSnapshot&) { shutdown_graphics(surfaces_[view]); },
                .event = [this, view](const host::WindowEvent& value) { event(surfaces_[view], value); }
            });
            if (created) surface.context = *created;
            else shutdown_graphics(surface);
            return created;
        }

    public:
        ~Application() override { epoch_gui_demo_destroy(renderer_); }
        host::ApplicationConfig configure(const host::ApplicationLaunchInfo&) override
        {
            // One Platform event/retirement owner; no secondary message pump.
            return {.runtime = {.dedicated_driver_threads = false}, .quit_when_no_active_contexts = true};
        }
        host::Status on_start(host::Runtime& runtime, const host::ApplicationLaunchInfo&) override
        {
            auto created = create_surface(runtime, 0, "EpochGui - Platform-owned native window", 1280, 820);
            if (!created) return std::unexpected(created.error());
            const HWND window = surfaces_[0].window;
            MONITORINFO monitor{sizeof(MONITORINFO)};
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                RECT outer{}, client{};
                if (GetWindowRect(window, &outer) && GetClientRect(window, &client))
                {
                    const auto preferred = epochgui_demo::win32::preferred_canvas_client_size(
                        GetDpiForWindow(window), monitor.rcWork.right - monitor.rcWork.left,
                        monitor.rcWork.bottom - monitor.rcWork.top,
                        (outer.right - outer.left) - (client.right - client.left),
                        (outer.bottom - outer.top) - (client.bottom - client.top));
                    if (auto resized = runtime.resize(*created, preferred.cx, preferred.cy); !resized) return resized;
                }
            }
            return {};
        }
        bool on_frame(host::Runtime& runtime, std::chrono::nanoseconds) override
        {
            // Reconcile after Runtime::tick, outside its context traversal.
            const auto main = runtime.context(surfaces_[0].context);
            if (!main || main->close_requested || !surfaces_[0].window) return false;
            for (unsigned id = 1; id < surfaces_.size(); ++id)
            {
                auto& surface = surfaces_[id];
                if (surface.context && !surface.window) surface.context = {};
                epoch_gui_demo_view_info info{};
                const bool requested = epoch_gui_demo_external_view(renderer_, id, &info);
                if (surface.context)
                {
                    if (!requested)
                    {
                        surface.closing_by_model = true;
                        runtime.request_context_close(surface.context);
                    }
                }
                else if (requested)
                {
                    const std::string title = std::string{"EpochGui - "} + info.title;
                    const UINT dpi = GetDpiForWindow(surfaces_[0].window);
                    const int width = MulDiv(info.width, static_cast<int>(dpi), 96);
                    const int height = MulDiv(info.height, static_cast<int>(dpi), 96);
                    if (!create_surface(runtime, id, title.c_str(), width, height))
                    {
                        epoch_gui_demo_redock_view(renderer_, id);
                        epoch_gui_demo_report_view_error(renderer_, "Could not open the native window; pane returned to the main window.");
                    }
                }
            }
            return true;
        }
        void on_stop(host::Runtime& runtime) noexcept override
        {
            // Retire child graphics on their own current context before the model.
            for (unsigned id = 1; id < surfaces_.size(); ++id)
                if (surfaces_[id].context) (void)runtime.destroy_context(surfaces_[id].context);
            if (surfaces_[0].context) (void)runtime.destroy_context(surfaces_[0].context);
        }
    };
    host::IApplication* EPOCH_PLATFORM_CALL create(const host::ApplicationLaunchInfo&) noexcept
    { return new (std::nothrow) Application; }
    void EPOCH_PLATFORM_CALL destroy(host::IApplication* app) noexcept { delete app; }
}

extern "C" void EPOCH_PLATFORM_CALL epoch_platform_application_factory(
    epochengine::platform::ApplicationFactory* factory) noexcept
{
    if (factory) *factory = {&create, &destroy};
}
