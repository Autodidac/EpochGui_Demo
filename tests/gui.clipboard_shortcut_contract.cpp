// SPDX-License-Identifier: MIT
// Conversion and shortcut routing only: never read or change the operator clipboard.
#include "../src/platform/gui.wgl_helpers.hpp"

int main()
{
    namespace host = epochgui_demo::win32;
    for (const auto key : {'A', 'C', 'X', 'V', 'Z', 'Y'})
    {
        if (host::edit_shortcut_command(key, false, false, true) != -1
            || host::edit_shortcut_command(key, true, false, false) != -1
            || host::edit_shortcut_command(key, true, false, true, true) != -1) return 1;
    }
    if (host::edit_shortcut_command('A', true, false, true) != 0
        || host::edit_shortcut_command('C', true, false, true) != 1
        || host::edit_shortcut_command('X', true, false, true) != 2
        || host::edit_shortcut_command('V', true, false, true) != 3
        || host::edit_shortcut_command('Z', true, false, true) != 4
        || host::edit_shortcut_command('Z', true, true, true) != 5
        || host::edit_shortcut_command('Y', true, false, true) != 5
        || host::edit_shortcut_command('Q', true, false, true) != -1) return 2;
    host::QueuedModifiers queued;
    queued.key(VK_CONTROL, true);
    if (host::edit_shortcut_command('V', queued.control, queued.shift, true) != 3) return 7;
    queued.key(VK_SHIFT, true);
    if (host::edit_shortcut_command('Z', queued.control, queued.shift, true) != 5) return 8;
    queued.key(VK_CONTROL, false);
    queued.key(VK_SHIFT, false);
    if (queued.control || queued.shift
        || host::edit_shortcut_command('V', queued.control, queued.shift, true) != -1) return 9;
    std::wstring wide;
    std::string utf8;
    const std::string original = "line one\n\xc3\xa9 \xf0\x9f\x98\x80\n";
    if (!host::clipboard_utf16_from_utf8(original, wide)
        || wide != L"line one\r\n\u00e9 \U0001f600\r\n"
        || !host::clipboard_utf8_from_utf16(wide, utf8) || utf8 != original) return 3;
    if (host::clipboard_utf16_from_utf8("\xc0\xaf", wide)
        || host::clipboard_utf8_from_utf16(std::wstring(1, wchar_t{0xd800}), utf8)) return 4;
    if (host::clipboard_utf16_from_utf8(std::string(host::maximum_clipboard_bytes + 1, 'x'), wide)
        || host::clipboard_utf8_from_utf16(std::wstring(host::maximum_clipboard_bytes, L'x'), utf8)) return 5;
    if (!host::clipboard_utf16_from_utf8({}, wide) || !wide.empty()
        || !host::clipboard_utf8_from_utf16({}, utf8) || !utf8.empty()) return 6;
    return 0;
}
