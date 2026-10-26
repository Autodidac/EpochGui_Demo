// SPDX-License-Identifier: MIT
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include "epochgui_demo/app_bridge.hpp"
namespace epochgui_demo::win32
{
    // Convert the catalog's logical canvas once at native creation. Include
    // non-client chrome in the work-area limit; never resize through a second host.
    [[nodiscard]] inline SIZE preferred_canvas_client_size(
        UINT dpi, int work_width, int work_height, int chrome_width, int chrome_height) noexcept
    {
        const int scale = static_cast<int>(dpi ? dpi : 96u);
        const int available_width = (std::max)(1, work_width - chrome_width);
        const int available_height = (std::max)(1, work_height - chrome_height);
        const int width = (std::min)(MulDiv(1280, scale, 96), available_width);
        const int height = (std::min)(MulDiv(820, scale, 96), available_height);
        return {width, height};
    }

    constexpr int WGL_CONTEXT_MAJOR_VERSION_ARB = 0x2091;
    constexpr int WGL_CONTEXT_MINOR_VERSION_ARB = 0x2092;
    constexpr int WGL_CONTEXT_FLAGS_ARB = 0x2094;
    constexpr int WGL_CONTEXT_PROFILE_MASK_ARB = 0x9126;
    constexpr int WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x0002;
    constexpr int WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001;
    constexpr UINT apply_native_frame_message = WM_APP + 1U;
    constexpr UINT render_frame_message = WM_APP + 2U;

    constexpr DWORD framed_style = WS_OVERLAPPEDWINDOW;

    using WglCreateContextAttribs = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
    using WglSwapInterval = BOOL(WINAPI*)(int);

    [[nodiscard]] inline bool invalid_wgl_address(epoch_gui_demo_gl_proc address) noexcept
    {
        const auto value = reinterpret_cast<std::uintptr_t>(address);
        return address == nullptr
            || value == 1U
            || value == 2U
            || value == 3U
            || value == static_cast<std::uintptr_t>(-1);
    }

    [[nodiscard]] inline epoch_gui_demo_gl_proc system_opengl_proc(const char* name) noexcept
    {
        if (std::strcmp(name, "glClearColor") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glClearColor);
        if (std::strcmp(name, "glClear") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glClear);
        if (std::strcmp(name, "glViewport") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glViewport);
        if (std::strcmp(name, "glDrawArrays") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glDrawArrays);
        if (std::strcmp(name, "glEnable") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glEnable);
        if (std::strcmp(name, "glDisable") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glDisable);
        if (std::strcmp(name, "glScissor") == 0)
            return reinterpret_cast<epoch_gui_demo_gl_proc>(&::glScissor);
        return nullptr;
    }

    [[nodiscard]] inline epoch_gui_demo_gl_proc load_opengl_proc(const char* name) noexcept
    {
        if (!name)
            return nullptr;
        if (const auto system_proc = system_opengl_proc(name))
            return system_proc;

        const auto address = reinterpret_cast<epoch_gui_demo_gl_proc>(wglGetProcAddress(name));
        return invalid_wgl_address(address) ? nullptr : address;
    }

    [[nodiscard]] inline int key_from_virtual_key(WPARAM key) noexcept
    {
        switch (key)
        {
        case VK_ESCAPE: return EPOCH_GUI_DEMO_KEY_ESCAPE;
        case VK_RETURN: return EPOCH_GUI_DEMO_KEY_ENTER;
        case VK_TAB: return EPOCH_GUI_DEMO_KEY_TAB;
        case VK_SPACE: return EPOCH_GUI_DEMO_KEY_SPACE;
        case VK_BACK: return EPOCH_GUI_DEMO_KEY_BACKSPACE;
        case VK_DELETE: return EPOCH_GUI_DEMO_KEY_DELETE;
        case VK_LEFT: return EPOCH_GUI_DEMO_KEY_LEFT;
        case VK_RIGHT: return EPOCH_GUI_DEMO_KEY_RIGHT;
        case VK_UP: return EPOCH_GUI_DEMO_KEY_UP;
        case VK_DOWN: return EPOCH_GUI_DEMO_KEY_DOWN;
        case VK_HOME: return EPOCH_GUI_DEMO_KEY_HOME;
        case VK_END: return EPOCH_GUI_DEMO_KEY_END;
        case VK_SHIFT: return EPOCH_GUI_DEMO_KEY_SHIFT;
        case VK_CONTROL: return EPOCH_GUI_DEMO_KEY_CONTROL;
        case VK_MENU: return EPOCH_GUI_DEMO_KEY_ALT;
        case VK_LWIN:
        case VK_RWIN:
            return EPOCH_GUI_DEMO_KEY_SUPER;
        default:
            return -1;
        }
    }

    inline constexpr std::size_t maximum_clipboard_bytes = 1024u * 1024u;

    [[nodiscard]] constexpr int edit_shortcut_command(
        WPARAM key, bool control, bool shift, bool down, bool repeated = false) noexcept
    {
        if (!control || !down || repeated) return -1;
        switch (key)
        {
        case 'A': return 0;
        case 'C': return 1;
        case 'X': return 2;
        case 'V': return 3;
        case 'Z': return shift ? 5 : 4;
        case 'Y': return 5;
        default: return -1;
        }
    }

    [[nodiscard]] inline bool clipboard_utf8_from_utf16(std::wstring_view text, std::string& output)
    {
        if (text.size() > maximum_clipboard_bytes / sizeof(wchar_t)) return false;
        output.clear();
        if (text.empty()) return true;
        const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (count <= 0 || static_cast<std::size_t>(count) > maximum_clipboard_bytes) return false;
        std::string converted(static_cast<std::size_t>(count), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), converted.data(), count, nullptr, nullptr) != count)
            return false;
        // The text controller uses LF internally; Windows clipboard text uses CRLF.
        output.reserve(converted.size());
        for (std::size_t index = 0; index < converted.size(); ++index)
            if (converted[index] != '\r' || index + 1 >= converted.size() || converted[index + 1] != '\n')
                output.push_back(converted[index]);
        return true;
    }

    [[nodiscard]] inline bool clipboard_utf16_from_utf8(std::string_view text, std::wstring& output)
    {
        if (text.size() > maximum_clipboard_bytes) return false;
        output.clear();
        if (text.empty()) return true;
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0) return false;
        std::wstring converted(static_cast<std::size_t>(count), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), converted.data(), count) != count)
            return false;
        output.reserve(converted.size());
        for (std::size_t index = 0; index < converted.size(); ++index)
        {
            if (converted[index] == L'\n' && (index == 0 || converted[index - 1] != L'\r'))
                output.push_back(L'\r');
            output.push_back(converted[index]);
        }
        return output.size() < maximum_clipboard_bytes / sizeof(wchar_t);
    }

    struct ClipboardSession
    {
        bool open{OpenClipboard(GetActiveWindow()) != FALSE};
        ~ClipboardSession() { if (open) CloseClipboard(); }
        ClipboardSession() = default;
        ClipboardSession(const ClipboardSession&) = delete;
        ClipboardSession& operator=(const ClipboardSession&) = delete;
    };

    [[nodiscard]] inline bool read_clipboard_utf8(std::string& output)
    {
        ClipboardSession session;
        if (!session.open || !IsClipboardFormatAvailable(CF_UNICODETEXT)) return false;
        const HANDLE data = GetClipboardData(CF_UNICODETEXT);
        if (!data) return false;
        const SIZE_T bytes = GlobalSize(data);
        if (bytes < sizeof(wchar_t) || bytes > maximum_clipboard_bytes) return false;
        const auto* text = static_cast<const wchar_t*>(GlobalLock(data));
        if (!text) return false;
        struct Unlock { HANDLE value; ~Unlock() { GlobalUnlock(value); } } unlock{data};
        const std::size_t capacity = bytes / sizeof(wchar_t);
        std::size_t length{};
        while (length < capacity && text[length] != L'\0') ++length;
        if (length == capacity) return false;
        return clipboard_utf8_from_utf16({text, length}, output);
    }

    [[nodiscard]] inline bool write_clipboard_utf8(std::string_view text)
    {
        std::wstring converted;
        if (!clipboard_utf16_from_utf8(text, converted)) return false;
        HANDLE data = GlobalAlloc(GMEM_MOVEABLE, (converted.size() + 1u) * sizeof(wchar_t));
        if (!data) return false;
        struct Release { HANDLE value; ~Release() { if (value) GlobalFree(value); } } release{data};
        auto* target = static_cast<wchar_t*>(GlobalLock(data));
        if (!target) return false;
        std::memcpy(target, converted.c_str(), (converted.size() + 1u) * sizeof(wchar_t));
        GlobalUnlock(data);
        ClipboardSession session;
        if (!session.open || !EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, data)) return false;
        release.value = nullptr; // Windows now owns the movable allocation.
        return true;
    }

    inline void configure_renderer_clipboard(epoch_gui_demo_renderer* renderer) noexcept
    {
        if (!renderer) return;
        try
        {
            epoch_gui_demo_set_clipboard(renderer,
                +[](void*) noexcept -> const char*
                {
                    try
                    {
                        thread_local std::string text;
                        return read_clipboard_utf8(text) ? text.c_str() : nullptr;
                    }
                    catch (...) { return nullptr; }
                },
                +[](void*, const char* text) noexcept -> bool
                {
                    if (!text) return false;
                    try
                    {
                        std::size_t bytes{};
                        while (bytes < maximum_clipboard_bytes && text[bytes] != '\0') ++bytes;
                        return bytes < maximum_clipboard_bytes && write_clipboard_utf8({text, bytes});
                    }
                    catch (...) { return false; }
                }, nullptr);
        }
        catch (...) { }
    }

    // Platform callbacks arrive after the native message queue has been drained.
    // Preserve modifier order from those events instead of sampling later OS state.
    struct QueuedModifiers
    {
        bool shift{}, control{}, alt{}, super{};
        void key(WPARAM value, bool down) noexcept
        {
            if (value == VK_SHIFT || value == VK_LSHIFT || value == VK_RSHIFT) shift = down;
            if (value == VK_CONTROL || value == VK_LCONTROL || value == VK_RCONTROL) control = down;
            if (value == VK_MENU || value == VK_LMENU || value == VK_RMENU) alt = down;
            if (value == VK_LWIN || value == VK_RWIN) super = down;
        }
    };

    [[nodiscard]] inline bool dispatch_edit_shortcut_with_modifiers(epoch_gui_demo_renderer* renderer,
        WPARAM virtual_key, bool down, bool repeated, const QueuedModifiers& modifiers) noexcept
    {
        if (!renderer || modifiers.alt) return false;
        const int command = edit_shortcut_command(virtual_key, modifiers.control,
            modifiers.shift, down, repeated);
        if (command < 0) return false;
        try { (void)epoch_gui_demo_edit_command(renderer, command, nullptr); return true; }
        catch (...) { return false; }
    }

    [[nodiscard]] inline bool dispatch_edit_shortcut(epoch_gui_demo_renderer* renderer,
        WPARAM virtual_key, bool down, bool repeated = false) noexcept
    {
        if (!renderer || (GetKeyState(VK_MENU) & 0x8000) != 0) return false;
        const int command = edit_shortcut_command(virtual_key,
            (GetKeyState(VK_CONTROL) & 0x8000) != 0,
            (GetKeyState(VK_SHIFT) & 0x8000) != 0, down, repeated);
        if (command < 0) return false;
        try
        {
            // Buttons, context menus and keys share the same renderer edit path.
            // Its clipboard writer must succeed before a cut changes the document.
            (void)epoch_gui_demo_edit_command(renderer, command, nullptr);
            return true;
        }
        catch (...) { return false; }
    }

    [[nodiscard]] inline LRESULT native_hit_test_from(int region) noexcept
    {
        switch (region)
        {
        case EPOCH_GUI_DEMO_WINDOW_CAPTION: return HTCAPTION;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_LEFT: return HTLEFT;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_RIGHT: return HTRIGHT;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP: return HTTOP;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM: return HTBOTTOM;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP_LEFT: return HTTOPLEFT;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP_RIGHT: return HTTOPRIGHT;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM_LEFT: return HTBOTTOMLEFT;
        case EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM_RIGHT: return HTBOTTOMRIGHT;
        default:
            return HTCLIENT;
        }
    }

    [[nodiscard]] inline bool create_opengl_context(HDC device_context, HGLRC& render_context)
        {
            PIXELFORMATDESCRIPTOR descriptor{};
            descriptor.nSize = sizeof(descriptor);
            descriptor.nVersion = 1;
            descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            descriptor.iPixelType = PFD_TYPE_RGBA;
            descriptor.cColorBits = 32;
            descriptor.cAlphaBits = 8;
            descriptor.cDepthBits = 24;
            descriptor.cStencilBits = 8;
            descriptor.iLayerType = PFD_MAIN_PLANE;

            const int pixel_format = ChoosePixelFormat(device_context, &descriptor);
            if (pixel_format == 0 || !SetPixelFormat(device_context, pixel_format, &descriptor))
                return false;

            HGLRC legacy_context = wglCreateContext(device_context);
            if (!legacy_context || !wglMakeCurrent(device_context, legacy_context))
            {
                if (legacy_context)
                    wglDeleteContext(legacy_context);
                return false;
            }

            const auto create_context = reinterpret_cast<WglCreateContextAttribs>(
                wglGetProcAddress("wglCreateContextAttribsARB"));
            if (!create_context)
            {
                wglMakeCurrent(nullptr, nullptr);
                wglDeleteContext(legacy_context);
                return false;
            }

            constexpr int attributes[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            render_context = create_context(device_context, nullptr, attributes);
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(legacy_context);
            if (!render_context || !wglMakeCurrent(device_context, render_context))
                return false;

            const auto swap_interval = reinterpret_cast<WglSwapInterval>(
                wglGetProcAddress("wglSwapIntervalEXT"));
            if (swap_interval)
                swap_interval(1);
            return true;
        }

}
