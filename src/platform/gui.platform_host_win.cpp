// SPDX-License-Identifier: MIT
#include "gui.wgl_helpers.hpp"
#include <epochplatformengine/epoch_platform_engine.hpp>
#include <chrono>
#include <expected>
#include <new>

namespace
{
    namespace host = epochengine::platform;
    class Application final : public host::IApplication
    {
        HWND window_{};
        HDC dc_{};
        HGLRC gl_{};
        epoch_gui_demo_renderer* renderer_{};
        wchar_t high_surrogate_{};
        unsigned held_buttons_{};
        epochgui_demo::win32::QueuedModifiers modifiers_{};

        void shutdown_graphics() noexcept
        {
            if (gl_ && dc_) wglMakeCurrent(dc_, gl_);
            epoch_gui_demo_destroy(renderer_);
            renderer_ = nullptr;
            wglMakeCurrent(nullptr, nullptr);
            if (gl_) wglDeleteContext(gl_);
            gl_ = nullptr;
            if (dc_ && window_) ReleaseDC(window_, dc_);
            dc_ = nullptr;
            window_ = nullptr;
        }

        void event(const host::WindowEvent& event)
        {
            if (!renderer_) return;
            using enum host::WindowEventType;
            switch (event.type)
            {
            case resized:
                epoch_gui_demo_resize(renderer_, event.width, event.height);
                break;
            case mouse_moved:
                epoch_gui_demo_pointer_move(renderer_, static_cast<float>(event.x), static_cast<float>(event.y));
                break;
            case mouse_button_pressed:
            case mouse_button_released:
                epoch_gui_demo_pointer_move(renderer_, static_cast<float>(event.x), static_cast<float>(event.y));
                if (event.button >= 1 && event.button <= 3)
                {
                    const auto bit = 1u << (event.button - 1);
                    if (event.type == mouse_button_pressed) held_buttons_ |= bit;
                    else held_buttons_ &= ~bit;
                    epoch_gui_demo_pointer_button(renderer_, event.button - 1, event.type == mouse_button_pressed);
                    if (held_buttons_) SetCapture(window_);
                    else if (GetCapture() == window_) ReleaseCapture();
                }
                break;
            case mouse_wheel:
                epoch_gui_demo_pointer_move(renderer_, static_cast<float>(event.x), static_cast<float>(event.y));
                epoch_gui_demo_wheel(renderer_, 0.0f, static_cast<float>(event.wheel_delta) / 120.0f);
                break;
            case key_pressed:
            case key_released:
                modifiers_.key(event.key, event.type == key_pressed);
                epoch_gui_demo_modifiers(renderer_, modifiers_.shift, modifiers_.control,
                    modifiers_.alt, modifiers_.super);
                if (epochgui_demo::win32::dispatch_edit_shortcut_with_modifiers(renderer_, event.key,
                        event.type == key_pressed, event.repeated, modifiers_)) break;
                if (const int key = epochgui_demo::win32::key_from_virtual_key(event.key); key >= 0)
                    epoch_gui_demo_key_event(renderer_, key, event.type == key_pressed, event.repeated);
                break;
            case text_input:
            {
                if (event.codepoint < 32 || event.codepoint == 127) break;
                const wchar_t unit = static_cast<wchar_t>(event.codepoint);
                if (unit >= 0xd800 && unit <= 0xdbff) { high_surrogate_ = unit; break; }
                wchar_t units[2]{unit, 0};
                int count = 1;
                if (high_surrogate_ && unit >= 0xdc00 && unit <= 0xdfff)
                { units[0] = high_surrogate_; units[1] = unit; count = 2; }
                high_surrogate_ = 0;
                char utf8[9]{};
                const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, units, count, utf8, 8, nullptr, nullptr);
                if (bytes > 0) epoch_gui_demo_text_input(renderer_, utf8);
                break;
            }
            case focus_lost:
                modifiers_ = {};
                held_buttons_ = 0;
                if (GetCapture() == window_) ReleaseCapture();
                for (int key = EPOCH_GUI_DEMO_KEY_ESCAPE; key <= EPOCH_GUI_DEMO_KEY_SUPER; ++key)
                    epoch_gui_demo_key_event(renderer_, key, false, false);
                for (int button = 0; button < 3; ++button) epoch_gui_demo_pointer_button(renderer_, button, false);
                epoch_gui_demo_modifiers(renderer_, false, false, false, false);
                high_surrogate_ = 0;
                break;
            default: break;
            }
        }

    public:
        host::ApplicationConfig configure(const host::ApplicationLaunchInfo&) override
        {
            // The canonical driver owns creation, event delivery and retirement.
            // GUI and WGL callbacks stay on its caller thread, with no second pump.
            return {.runtime = {.dedicated_driver_threads = false}, .quit_when_no_active_contexts = true};
        }
        host::Status on_start(host::Runtime& runtime, const host::ApplicationLaunchInfo&) override
        {
            auto created = runtime.create_context({.driver = host::DriverKind::win32,
                .type = host::ContextType::opengl, .thread_policy = host::ThreadPolicy::caller_thread,
                .window = {.title = "EpochGui - Platform-owned native window", .width = 1280, .height = 820},
                .route = "gui-demo"}, {
                .initialize = [this](const host::ContextSnapshot& snapshot) -> host::Status {
                    window_ = static_cast<HWND>(snapshot.native.window);
                    dc_ = GetDC(window_);
                    if (!dc_ || !epochgui_demo::win32::create_opengl_context(dc_, gl_))
                        return std::unexpected(host::Error{host::ErrorCode::initialization_failed, "WGL initialization failed"});
                    renderer_ = epoch_gui_demo_create();
                    if (!renderer_ || !epoch_gui_demo_initialize(renderer_, &epochgui_demo::win32::load_opengl_proc))
                        return std::unexpected(host::Error{host::ErrorCode::initialization_failed, "GUI renderer initialization failed"});
                    epochgui_demo::win32::configure_renderer_clipboard(renderer_);
                    RECT client{};
                    GetClientRect(window_, &client);
                    epoch_gui_demo_resize(renderer_, client.right, client.bottom);
                    return {};
                },
                .frame = [this](const host::ContextSnapshot&, std::chrono::nanoseconds) {
                    if (!renderer_) return false;
                    epoch_gui_demo_render(renderer_);
                    SwapBuffers(dc_);
                    const int command = epoch_gui_demo_take_window_command(renderer_);
                    if (command == EPOCH_GUI_DEMO_COMMAND_CLOSE) return false;
                    if (command == EPOCH_GUI_DEMO_COMMAND_MINIMIZE) ShowWindow(window_, SW_MINIMIZE);
                    if (command == EPOCH_GUI_DEMO_COMMAND_TOGGLE_MAXIMIZE)
                        ShowWindow(window_, IsZoomed(window_) ? SW_RESTORE : SW_MAXIMIZE);
                    return true;
                },
                .shutdown = [this](const host::ContextSnapshot&) { shutdown_graphics(); },
                .event = [this](const host::WindowEvent& value) { event(value); }
            });
            if (!created) return std::unexpected(created.error());
            MONITORINFO monitor{sizeof(MONITORINFO)};
            if (GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                RECT outer{}, client{};
                if (GetWindowRect(window_, &outer) && GetClientRect(window_, &client))
                {
                    const auto preferred = epochgui_demo::win32::preferred_canvas_client_size(
                        GetDpiForWindow(window_),
                        monitor.rcWork.right - monitor.rcWork.left,
                        monitor.rcWork.bottom - monitor.rcWork.top,
                        (outer.right - outer.left) - (client.right - client.left),
                        (outer.bottom - outer.top) - (client.bottom - client.top));
                    if (auto resized = runtime.resize(*created, preferred.cx, preferred.cy); !resized)
                        return resized;
                }
            }
            return {};
        }
        bool on_frame(host::Runtime&, std::chrono::nanoseconds) override { return true; }
        void on_stop(host::Runtime&) noexcept override { shutdown_graphics(); }
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
