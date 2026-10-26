#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*epoch_gui_demo_gl_proc)(void);
    typedef epoch_gui_demo_gl_proc (*epoch_gui_demo_proc_loader)(const char* name);

    enum epoch_gui_demo_pointer_button
    {
        EPOCH_GUI_DEMO_POINTER_LEFT = 0,
        EPOCH_GUI_DEMO_POINTER_RIGHT = 1,
        EPOCH_GUI_DEMO_POINTER_MIDDLE = 2,
        EPOCH_GUI_DEMO_POINTER_AUXILIARY_1 = 3,
        EPOCH_GUI_DEMO_POINTER_AUXILIARY_2 = 4
    };

    enum epoch_gui_demo_key
    {
        EPOCH_GUI_DEMO_KEY_ESCAPE = 0,
        EPOCH_GUI_DEMO_KEY_ENTER = 1,
        EPOCH_GUI_DEMO_KEY_TAB = 2,
        EPOCH_GUI_DEMO_KEY_SPACE = 3,
        EPOCH_GUI_DEMO_KEY_BACKSPACE = 4,
        EPOCH_GUI_DEMO_KEY_DELETE = 5,
        EPOCH_GUI_DEMO_KEY_LEFT = 6,
        EPOCH_GUI_DEMO_KEY_RIGHT = 7,
        EPOCH_GUI_DEMO_KEY_UP = 8,
        EPOCH_GUI_DEMO_KEY_DOWN = 9,
        EPOCH_GUI_DEMO_KEY_HOME = 10,
        EPOCH_GUI_DEMO_KEY_END = 11,
        EPOCH_GUI_DEMO_KEY_SHIFT = 12,
        EPOCH_GUI_DEMO_KEY_CONTROL = 13,
        EPOCH_GUI_DEMO_KEY_ALT = 14,
        EPOCH_GUI_DEMO_KEY_SUPER = 15
    };

    enum epoch_gui_demo_window_region
    {
        EPOCH_GUI_DEMO_WINDOW_OUTSIDE = 0,
        EPOCH_GUI_DEMO_WINDOW_CLIENT = 1,
        EPOCH_GUI_DEMO_WINDOW_CAPTION = 2,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_LEFT = 3,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_RIGHT = 4,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP = 5,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM = 6,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP_LEFT = 7,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_TOP_RIGHT = 8,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM_LEFT = 9,
        EPOCH_GUI_DEMO_WINDOW_RESIZE_BOTTOM_RIGHT = 10,
        EPOCH_GUI_DEMO_WINDOW_MINIMIZE = 11,
        EPOCH_GUI_DEMO_WINDOW_MAXIMIZE = 12,
        EPOCH_GUI_DEMO_WINDOW_CLOSE = 13
    };

    enum epoch_gui_demo_window_command
    {
        EPOCH_GUI_DEMO_COMMAND_NONE = 0,
        EPOCH_GUI_DEMO_COMMAND_MINIMIZE = 1,
        EPOCH_GUI_DEMO_COMMAND_TOGGLE_MAXIMIZE = 2,
        EPOCH_GUI_DEMO_COMMAND_CLOSE = 3
    };

    enum epoch_gui_demo_native_frame_mode
    {
        EPOCH_GUI_DEMO_FRAME_MODE_NONE = -1,
        EPOCH_GUI_DEMO_FRAME_MODE_BORDERLESS = 0,
        EPOCH_GUI_DEMO_FRAME_MODE_NATIVE = 1
    };

    struct epoch_gui_demo_renderer;

    // 0 Select all, 1 Copy, 2 Cut, 3 Paste, 4 Undo, 5 Redo.
    // Returned UTF-8 remains valid until the next edit command or destruction.
    const char* epoch_gui_demo_edit_command(epoch_gui_demo_renderer* renderer,
        int command, const char* pasted_utf8);
    void epoch_gui_demo_set_clipboard(epoch_gui_demo_renderer* renderer,
        const char* (*reader)(void*), bool (*writer)(void*, const char*), void* user);

    [[nodiscard]] epoch_gui_demo_renderer* epoch_gui_demo_create(void);
    void epoch_gui_demo_destroy(epoch_gui_demo_renderer* renderer);

    [[nodiscard]] bool epoch_gui_demo_initialize(
        epoch_gui_demo_renderer* renderer,
        epoch_gui_demo_proc_loader loader);

    void epoch_gui_demo_resize(
        epoch_gui_demo_renderer* renderer,
        int width,
        int height);

    void epoch_gui_demo_pointer_move(
        epoch_gui_demo_renderer* renderer,
        float x,
        float y);

    void epoch_gui_demo_pointer_button(
        epoch_gui_demo_renderer* renderer,
        int button,
        bool down);

    void epoch_gui_demo_wheel(
        epoch_gui_demo_renderer* renderer,
        float horizontal,
        float vertical);

    void epoch_gui_demo_key_event(
        epoch_gui_demo_renderer* renderer,
        int key,
        bool down,
        bool repeated);

    void epoch_gui_demo_modifiers(
        epoch_gui_demo_renderer* renderer,
        bool shift,
        bool control,
        bool alt,
        bool super_key);

    void epoch_gui_demo_text_input(
        epoch_gui_demo_renderer* renderer,
        const char* utf8_text);

    [[nodiscard]] int epoch_gui_demo_window_hit_test(
        const epoch_gui_demo_renderer* renderer,
        float x,
        float y);

    [[nodiscard]] int epoch_gui_demo_take_window_command(
        epoch_gui_demo_renderer* renderer);

    [[nodiscard]] int epoch_gui_demo_take_native_frame_mode(
        epoch_gui_demo_renderer* renderer);

    void epoch_gui_demo_render(epoch_gui_demo_renderer* renderer);

#ifdef __cplusplus
}
#endif
