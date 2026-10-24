// SPDX-License-Identifier: MIT
#include "../src/platform/gui.wgl_helpers.hpp"
int main()
{
    for (UINT dpi : {96u,120u,144u,192u}) {
        const auto full = epochgui_demo::win32::preferred_canvas_client_size(dpi,4000,2400,16,40);
        if (full.cx != MulDiv(1280,dpi,96) || full.cy != MulDiv(820,dpi,96)) return 1;
        const auto laptop = epochgui_demo::win32::preferred_canvas_client_size(dpi,1366,728,16,40);
        if (laptop.cx > 1350 || laptop.cy > 688 || laptop.cx < 1 || laptop.cy < 1) return 2;
    }
    const auto tiny = epochgui_demo::win32::preferred_canvas_client_size(144,10,20,16,40);
    if (tiny.cx != 1 || tiny.cy != 1) return 3;
    const auto fallback = epochgui_demo::win32::preferred_canvas_client_size(0,2000,1200,16,40);
    return fallback.cx == 1280 && fallback.cy == 820 ? 0 : 4;
}
