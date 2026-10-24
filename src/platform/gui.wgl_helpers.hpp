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
#include <cstdint>
#include <cstring>
#include "epochgui_demo/app_bridge.hpp"
namespace epochgui_demo::win32
{
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
