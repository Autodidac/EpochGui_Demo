module;

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

export module epoch.gui.demo.opengl;

export import epoch.gui.rounded_rect;
export import epoch.gui.input;

namespace epochengine::gui_lib
{
    [[nodiscard]] inline SplitterLayoutOptions visible_demo_splitter_options(
        float split_fraction) noexcept
    {
        return {
            .area = { { 250.0f, 610.0f }, { 940.0f, 142.0f } },
            .axis = SplitterAxis::vertical,
            .split_fraction = std::clamp(split_fraction, 0.0f, 1.0f),
            .thickness = 8.0f,
            .min_before = 180.0f,
            .min_after = 260.0f
        };
    }

    [[nodiscard]] inline float splitter_fraction_from_update_layout(
        const SplitterLayout& layout) noexcept
    {
        constexpr float update_usable_width = 970.0f - 8.0f;
        return update_usable_width > 0.0f
            ? std::clamp(layout.before.size.x / update_usable_width, 0.0f, 1.0f)
            : 0.5f;
    }

    [[nodiscard]] inline bool splitter_hit_test_with_capture(
        const SplitterLayout& update_layout,
        Vec2 pointer_position,
        float hit_slop,
        bool pointer_pressed) noexcept
    {
        static bool dragging{};
        if (pointer_pressed)
        {
            const SplitterLayout visible_layout = make_splitter_layout(
                visible_demo_splitter_options(
                    splitter_fraction_from_update_layout(update_layout)));
            dragging = splitter_hit_test(
                visible_layout,
                pointer_position,
                hit_slop);
        }
        return dragging;
    }

    [[nodiscard]] inline float splitter_fraction_from_visible_point(
        const SplitterLayoutOptions&,
        Vec2 pointer_position) noexcept
    {
        return splitter_fraction_from_point(
            visible_demo_splitter_options(0.5f),
            pointer_position);
    }
}

export namespace epochengine::gui_demo
{
    namespace rounded_rect = epochengine::gui_lib::rounded_rect;
    namespace input = epochengine::gui_lib::input;

    using OpenGLProc = void (*)(void);
    using OpenGLProcLoader = OpenGLProc (*)(const char* name);
    using PointerButton = input::PointerButton;
    using Key = input::Key;
    using WindowChromeRegion = input::WindowChromeRegion;
    using WindowCommand = input::WindowCommand;

    struct ExternalViewInfo
    {
        unsigned id{};
        std::string_view title{};
        int width{};
        int height{};
        bool attached{};
    };

    class OpenGLRenderer final
    {
    public:
        OpenGLRenderer();
        ~OpenGLRenderer();

        OpenGLRenderer(OpenGLRenderer&&) noexcept;
        OpenGLRenderer& operator=(OpenGLRenderer&&) noexcept;

        OpenGLRenderer(const OpenGLRenderer&) = delete;
        OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

        [[nodiscard]] bool initialize(OpenGLProcLoader loader) noexcept;
        void resize(int width, int height) noexcept;
        void pointer_move(float x, float y) noexcept;
        void pointer_button(PointerButton button, bool down) noexcept;
        void cancel_input() noexcept;
        [[nodiscard]] bool external_view(unsigned id, ExternalViewInfo& info) const noexcept;
        [[nodiscard]] bool initialize_view(unsigned id, OpenGLProcLoader loader) noexcept;
        void release_view(unsigned id) noexcept; // Caller makes that view's graphics context current.
        void release_primary() noexcept;
        void redock_view(unsigned id) noexcept;
        void resize_view(unsigned id, int width, int height, float dpi_scale) noexcept;
        void set_input_source(unsigned id) noexcept;
        void pointer_move_view(unsigned id, float x, float y) noexcept;
        [[nodiscard]] bool render_view(unsigned id) noexcept;
        void report_view_error(std::string_view message);

        void wheel(float horizontal, float vertical) noexcept;
        void key(Key key_code, bool down, bool repeated = false) noexcept;
        void modifiers(input::ModifierState state) noexcept;
        void text_input(std::string_view text);
        [[nodiscard]] std::string edit_command(int command, std::string_view pasted_text = {});
        void set_clipboard(const char* (*reader)(void*), bool (*writer)(void*, const char*), void* user) noexcept;
        void render() noexcept;

        [[nodiscard]] WindowChromeRegion window_chrome_hit_test(float x, float y) const noexcept;
        [[nodiscard]] WindowCommand take_window_command() noexcept;
        [[nodiscard]] int take_native_frame_mode() noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
