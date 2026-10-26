#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

#include <GL/gl.h>

#include "epochgui_demo/app_bridge.hpp"
#include "gui.wgl_helpers.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace
{
    using namespace epochgui_demo::win32;

    class WindowsApplication final
    {
    public:
        ~WindowsApplication()
        {
            shutdown_graphics();
        }

        [[nodiscard]] bool initialize(HINSTANCE instance)
        {
            instance_ = instance;
            constexpr wchar_t class_name[] = L"EpochGuiCompleteDemoOpenGL";

            WNDCLASSEXW window_class{};
            window_class.cbSize = sizeof(window_class);
            window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            window_class.lpfnWndProc = &WindowsApplication::window_proc;
            window_class.hInstance = instance_;
            window_class.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(101));
            if (!window_class.hIcon)
                window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            window_class.lpszClassName = class_name;
            window_class.hIconSm = LoadIconW(instance_, MAKEINTRESOURCEW(101));
            if (!window_class.hIconSm)
                window_class.hIconSm = window_class.hIcon;

            if (!RegisterClassExW(&window_class)
                && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }

            RECT frame{ 0, 0, 1280, 820 };
            if (!AdjustWindowRectEx(&frame, framed_style, FALSE, 0))
                return false;

            const int width = frame.right - frame.left;
            const int height = frame.bottom - frame.top;
            const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
            const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

            window_ = CreateWindowExW(
                0,
                class_name,
                L"EpochGui Complete Demo",
                framed_style,
                x,
                y,
                width,
                height,
                nullptr,
                nullptr,
                instance_,
                this);
            if (!window_)
                return false;

            // Match the Platform host's logical catalog size without depending
            // on Platform: this native host retains ownership of its HWND.
            MONITORINFO monitor{sizeof(MONITORINFO)};
            RECT outer{}, client{};
            if (GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &monitor)
                && GetWindowRect(window_, &outer) && GetClientRect(window_, &client))
            {
                const int chrome_width = (outer.right - outer.left) - (client.right - client.left);
                const int chrome_height = (outer.bottom - outer.top) - (client.bottom - client.top);
                const auto preferred = epochgui_demo::win32::preferred_canvas_client_size(
                    GetDpiForWindow(window_), monitor.rcWork.right - monitor.rcWork.left,
                    monitor.rcWork.bottom - monitor.rcWork.top, chrome_width, chrome_height);
                const int outer_width = preferred.cx + chrome_width;
                const int outer_height = preferred.cy + chrome_height;
                if (!SetWindowPos(window_, nullptr,
                        monitor.rcWork.left + (monitor.rcWork.right - monitor.rcWork.left - outer_width) / 2,
                        monitor.rcWork.top + (monitor.rcWork.bottom - monitor.rcWork.top - outer_height) / 2,
                        outer_width, outer_height, SWP_NOZORDER | SWP_NOACTIVATE))
                    return false;
            }
            apply_dwm_frame();
            device_context_ = GetDC(window_);
            if (!device_context_ || !create_opengl_context())
                return false;

            renderer_ = epoch_gui_demo_create();
            if (!renderer_ || !epoch_gui_demo_initialize(renderer_, &load_opengl_proc))
                return false;
            epochgui_demo::win32::configure_renderer_clipboard(renderer_);

            update_renderer_size_from_client();

            ShowWindow(window_, SW_SHOWNORMAL);
            UpdateWindow(window_);
            request_render();
            return true;
        }

        [[nodiscard]] int run()
        {
            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            return static_cast<int>(message.wParam);
        }

    private:
        [[nodiscard]] bool create_opengl_context()
        {
            return epochgui_demo::win32::create_opengl_context(device_context_, render_context_);
        }

        void shutdown_graphics()
        {
            if (graphics_shutdown_)
                return;
            graphics_shutdown_ = true;

            if (render_context_ && device_context_)
                wglMakeCurrent(device_context_, render_context_);

            epoch_gui_demo_destroy(renderer_);
            renderer_ = nullptr;

            wglMakeCurrent(nullptr, nullptr);
            if (render_context_)
            {
                wglDeleteContext(render_context_);
                render_context_ = nullptr;
            }
            if (device_context_ && window_)
            {
                ReleaseDC(window_, device_context_);
                device_context_ = nullptr;
            }
        }

        void apply_dwm_frame()
        {
            if (!window_)
                return;

            const DWMNCRENDERINGPOLICY policy = native_frame_enabled_
                ? DWMNCRP_USEWINDOWSTYLE
                : DWMNCRP_DISABLED;
            DwmSetWindowAttribute(
                window_,
                DWMWA_NCRENDERING_POLICY,
                &policy,
                sizeof(policy));

            BOOL allow_nc_paint = native_frame_enabled_ ? TRUE : FALSE;
            DwmSetWindowAttribute(
                window_,
                DWMWA_ALLOW_NCPAINT,
                &allow_nc_paint,
                sizeof(allow_nc_paint));

            const MARGINS margins = native_frame_enabled_
                ? MARGINS{ 0, 0, 0, 0 }
                : MARGINS{ 1, 1, 1, 1 };
            DwmExtendFrameIntoClientArea(window_, &margins);

            if (native_frame_enabled_)
            {
                (void)SetWindowTheme(window_, L"Explorer", nullptr);
                RedrawWindow(
                    window_,
                    nullptr,
                    nullptr,
                    RDW_INVALIDATE | RDW_FRAME | RDW_ERASE);
            }
        }

        void update_renderer_size_from_client()
        {
            if (!renderer_ || !window_)
                return;

            RECT client{};
            if (GetClientRect(window_, &client))
            {
                epoch_gui_demo_resize(
                    renderer_,
                    (std::max)(1L, client.right - client.left),
                    (std::max)(1L, client.bottom - client.top));
            }
        }

        void request_render()
        {
            if (!window_ || graphics_shutdown_ || render_message_pending_)
                return;
            render_message_pending_ = true;
            if (!PostMessageW(window_, render_frame_message, 0, 0))
                render_message_pending_ = false;
        }

        void request_native_frame_mode(bool enabled)
        {
            if (!window_ || graphics_shutdown_)
                return;

            pending_native_frame_enabled_ = enabled;
            if (frame_change_message_pending_)
                return;

            frame_change_message_pending_ = true;
            if (!PostMessageW(window_, apply_native_frame_message, 0, 0))
                frame_change_message_pending_ = false;
        }

        void apply_native_frame_mode(bool enabled)
        {
            if (!window_
                || graphics_shutdown_
                || frame_change_in_progress_
                || native_frame_enabled_ == enabled)
            {
                return;
            }

            frame_change_in_progress_ = true;
            native_frame_enabled_ = enabled;
            apply_dwm_frame();

            SetWindowPos(
                window_,
                nullptr,
                0,
                0,
                0,
                0,
                SWP_NOMOVE
                    | SWP_NOSIZE
                    | SWP_NOZORDER
                    | SWP_NOACTIVATE
                    | SWP_FRAMECHANGED);

            update_renderer_size_from_client();
            frame_change_in_progress_ = false;
            request_render();
        }

        void update_modifiers()
        {
            if (!renderer_)
                return;
            epoch_gui_demo_modifiers(
                renderer_,
                (GetKeyState(VK_SHIFT) & 0x8000) != 0,
                (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                (GetKeyState(VK_MENU) & 0x8000) != 0,
                (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0);
        }

        void render_and_apply_commands()
        {
            if (!renderer_
                || !device_context_
                || !render_context_
                || rendering_
                || frame_change_in_progress_
                || graphics_shutdown_)
            {
                return;
            }

            if (wglGetCurrentContext() != render_context_
                && !wglMakeCurrent(device_context_, render_context_))
            {
                return;
            }

            rendering_ = true;
            epoch_gui_demo_render(renderer_);
            SwapBuffers(device_context_);
            const int window_command = epoch_gui_demo_take_window_command(renderer_);
            const int frame_mode = epoch_gui_demo_take_native_frame_mode(renderer_);
            rendering_ = false;

            switch (window_command)
            {
            case EPOCH_GUI_DEMO_COMMAND_MINIMIZE:
                ShowWindow(window_, SW_MINIMIZE);
                break;
            case EPOCH_GUI_DEMO_COMMAND_TOGGLE_MAXIMIZE:
                ShowWindow(window_, IsZoomed(window_) ? SW_RESTORE : SW_MAXIMIZE);
                break;
            case EPOCH_GUI_DEMO_COMMAND_CLOSE:
                PostMessageW(window_, WM_CLOSE, 0, 0);
                break;
            default:
                break;
            }

            if (frame_mode == EPOCH_GUI_DEMO_FRAME_MODE_NATIVE)
                request_native_frame_mode(true);
            else if (frame_mode == EPOCH_GUI_DEMO_FRAME_MODE_BORDERLESS)
                request_native_frame_mode(false);
        }

        void feed_pointer_position(LPARAM lparam)
        {
            if (!renderer_)
                return;
            epoch_gui_demo_pointer_move(
                renderer_,
                static_cast<float>(GET_X_LPARAM(lparam)),
                static_cast<float>(GET_Y_LPARAM(lparam)));
        }

        void feed_pointer_button(LPARAM lparam, int button, bool down)
        {
            feed_pointer_position(lparam);
            epoch_gui_demo_pointer_button(renderer_, button, down);
            const auto bit = 1u << static_cast<unsigned>(button);
            if (down) held_buttons_ |= bit;
            else held_buttons_ &= ~bit;
            if (held_buttons_) SetCapture(window_);
            else if (GetCapture() == window_) ReleaseCapture();
            request_render();
        }

        void feed_character(wchar_t character)
        {
            if (!renderer_ || character < 32 || character == 127)
                return;

            std::array<char, 8> utf8{};
            const int written = WideCharToMultiByte(
                CP_UTF8,
                0,
                &character,
                1,
                utf8.data(),
                static_cast<int>(utf8.size() - 1),
                nullptr,
                nullptr);
            if (written <= 0)
                return;
            utf8[static_cast<std::size_t>(written)] = '\0';
            epoch_gui_demo_text_input(renderer_, utf8.data());
            request_render();
        }

        [[nodiscard]] LRESULT handle_message(
            HWND window,
            UINT message,
            WPARAM wparam,
            LPARAM lparam)
        {
            switch (message)
            {
            case render_frame_message:
                render_message_pending_ = false;
                render_and_apply_commands();
                return 0;

            case apply_native_frame_message:
                frame_change_message_pending_ = false;
                apply_native_frame_mode(pending_native_frame_enabled_);
                return 0;

            case WM_NCCALCSIZE:
                if (!native_frame_enabled_ && wparam != 0)
                    return 0;
                break;

            case WM_NCACTIVATE:
                if (!native_frame_enabled_)
                    return TRUE;
                break;

            case WM_NCPAINT:
                if (!native_frame_enabled_)
                    return 0;
                break;

            case WM_NCHITTEST:
            {
                if (frame_change_in_progress_)
                    return DefWindowProcW(window, message, wparam, lparam);

                if (native_frame_enabled_)
                {
                    const LRESULT native_result = DefWindowProcW(window, message, wparam, lparam);
                    if (native_result != HTCLIENT)
                        return native_result;
                }
                if (!renderer_)
                    break;
                POINT point{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
                ScreenToClient(window, &point);
                const int region = epoch_gui_demo_window_hit_test(
                    renderer_,
                    static_cast<float>(point.x),
                    static_cast<float>(point.y));
                return native_hit_test_from(region);
            }

            case WM_GETMINMAXINFO:
            {
                auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
                info->ptMinTrackSize.x = 880;
                info->ptMinTrackSize.y = 580;
                return 0;
            }

            case WM_PAINT:
            {
                PAINTSTRUCT paint{};
                BeginPaint(window, &paint);
                render_and_apply_commands();
                EndPaint(window, &paint);
                return 0;
            }

            case WM_SIZE:
                if (renderer_)
                {
                    epoch_gui_demo_resize(
                        renderer_,
                        (std::max)(1, static_cast<int>(LOWORD(lparam))),
                        (std::max)(1, static_cast<int>(HIWORD(lparam))));
                    request_render();
                }
                return 0;

            case WM_KILLFOCUS:
            case WM_CANCELMODE:
                held_buttons_ = 0;
                epoch_gui_demo_cancel_input(renderer_);
                if (GetCapture() == window_) ReleaseCapture();
                request_render();
                return 0;
            case WM_CAPTURECHANGED:
                if (held_buttons_ && reinterpret_cast<HWND>(lparam) != window_)
                {
                    held_buttons_ = 0;
                    epoch_gui_demo_cancel_input(renderer_);
                    request_render();
                }
                return 0;

            case WM_MOUSEMOVE:
                feed_pointer_position(lparam);
                request_render();
                return 0;

            case WM_LBUTTONDOWN:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_LEFT, true);
                return 0;
            case WM_LBUTTONUP:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_LEFT, false);
                return 0;
            case WM_RBUTTONDOWN:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_RIGHT, true);
                return 0;
            case WM_RBUTTONUP:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_RIGHT, false);
                return 0;
            case WM_MBUTTONDOWN:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_MIDDLE, true);
                return 0;
            case WM_MBUTTONUP:
                feed_pointer_button(lparam, EPOCH_GUI_DEMO_POINTER_MIDDLE, false);
                return 0;

            case WM_MOUSEWHEEL:
            {
                POINT point{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
                ScreenToClient(window, &point);
                epoch_gui_demo_pointer_move(renderer_, static_cast<float>(point.x), static_cast<float>(point.y));
                epoch_gui_demo_wheel(
                    renderer_,
                    0.0f,
                    static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA));
                request_render();
                return 0;
            }

            case WM_CHAR:
                feed_character(static_cast<wchar_t>(wparam));
                return 0;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
                const bool repeated = down && (lparam & (1LL << 30)) != 0;
                update_modifiers();
                if (epochgui_demo::win32::dispatch_edit_shortcut(renderer_, wparam, down, repeated))
                {
                    request_render();
                    return 0;
                }
                const int key = key_from_virtual_key(wparam);
                if (key >= 0 && renderer_)
                {
                    epoch_gui_demo_key_event(renderer_, key, down, repeated);
                    request_render();
                    return 0;
                }
                break;
            }

            case WM_ERASEBKGND:
                return 1;

            case WM_CLOSE:
                shutdown_graphics();
                DestroyWindow(window);
                return 0;

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            default:
                break;
            }

            return DefWindowProcW(window, message, wparam, lparam);
        }

        static LRESULT CALLBACK window_proc(
            HWND window,
            UINT message,
            WPARAM wparam,
            LPARAM lparam)
        {
            WindowsApplication* application = reinterpret_cast<WindowsApplication*>(
                GetWindowLongPtrW(window, GWLP_USERDATA));

            if (message == WM_NCCREATE)
            {
                const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
                application = static_cast<WindowsApplication*>(create->lpCreateParams);
                application->window_ = window;
                SetWindowLongPtrW(
                    window,
                    GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(application));
            }

            return application
                ? application->handle_message(window, message, wparam, lparam)
                : DefWindowProcW(window, message, wparam, lparam);
        }

        HINSTANCE instance_{};
        HWND window_{};
        HDC device_context_{};
        HGLRC render_context_{};
        epoch_gui_demo_renderer* renderer_{};
        bool native_frame_enabled_{ true };
        bool pending_native_frame_enabled_{ true };
        bool frame_change_message_pending_{};
        bool frame_change_in_progress_{};
        bool render_message_pending_{};
        bool rendering_{};
        unsigned held_buttons_{};
        bool graphics_shutdown_{};
    };
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WindowsApplication application;
    if (!application.initialize(instance))
    {
        MessageBoxW(
            nullptr,
            L"OpenGL 3.2 core initialization failed.",
            L"EpochGui Complete Demo",
            MB_OK | MB_ICONERROR);
        return 1;
    }
    return application.run();
}
