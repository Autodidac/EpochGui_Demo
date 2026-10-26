module;

#include <compare>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module epoch.gui.demo.opengl;

import epoch.gui;
import epoch.gui.rounded_rect;
import epoch.gui.input;
import epoch.gui.font;
import epoch.gui.image;
import gui.library_models;
import gui.text_view;
#if EPOCH_DEMO_MEDIA
import media.timeline_preview;
#endif

namespace epochengine::gui_demo
{
    namespace
    {
        namespace gui = epochengine::gui_lib;
        namespace rounded = epochengine::gui_lib::rounded_rect;
        namespace input = epochengine::gui_lib::input;
        namespace font = epochengine::gui_lib::font;
        namespace image = epochengine::gui_lib::image;

        using GLenum = unsigned int;
        using GLboolean = unsigned char;
        using GLbitfield = unsigned int;
        using GLint = int;
        using GLsizei = int;
        using GLuint = unsigned int;
        using GLfloat = float;
        using GLchar = char;
        using GLsizeiptr = std::ptrdiff_t;

        constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
        constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
        constexpr GLenum GL_FLOAT = 0x1406;
        constexpr GLenum GL_TRIANGLES = 0x0004;
        constexpr GLenum GL_SCISSOR_TEST = 0x0C11;
        constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
        constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
        constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
        constexpr GLenum GL_LINK_STATUS = 0x8B82;
        constexpr GLbitfield GL_COLOR_BUFFER_BIT = 0x00004000;
        constexpr GLboolean GL_FALSE = 0;

        constexpr float canvas_width = 1280.0f;
        constexpr float canvas_height = 820.0f;
        constexpr float custom_title_height = 54.0f;
        constexpr float toolbar_height = 58.0f;
        constexpr float workspace_top = custom_title_height + toolbar_height;
        constexpr float sidebar_width = 214.0f;

        struct Color
        {
            float r{};
            float g{};
            float b{};
            float a{ 1.0f };
        };

        [[nodiscard]] constexpr Color rgb(std::uint32_t value, float alpha = 1.0f) noexcept
        {
            return {
                static_cast<float>((value >> 16U) & 0xffU) / 255.0f,
                static_cast<float>((value >> 8U) & 0xffU) / 255.0f,
                static_cast<float>(value & 0xffU) / 255.0f,
                alpha
            };
        }

        constexpr Color background = rgb(0x0a0f1c);
        constexpr Color title_fill = rgb(0x111a2b);
        constexpr Color toolbar_fill = rgb(0x0e1728);
        constexpr Color sidebar_fill = rgb(0x10192a);
        constexpr Color workspace_fill = rgb(0x0f1827);
        constexpr Color panel_fill = rgb(0x141f33);
        constexpr Color panel_border = rgb(0x2a3b57);
        constexpr Color control_fill = rgb(0x1b2941);
        constexpr Color control_hover = rgb(0x263b5c);
        constexpr Color control_selected = rgb(0x315f91);
        constexpr Color accent = rgb(0x65a7ff);
        constexpr Color accent_soft = rgb(0x244d79);
        constexpr Color green = rgb(0x6ee7b7);
        constexpr Color warning = rgb(0xf2bd62);
        constexpr Color danger = rgb(0xef6b73);
        constexpr Color text_primary = rgb(0xeaf0fa);
        constexpr Color text_secondary = rgb(0xa9b9cf);
        constexpr Color text_muted = rgb(0x71839d);

        template <typename Function>
        [[nodiscard]] bool load_function(Function& function, OpenGLProcLoader loader, const char* name) noexcept
        {
            function = reinterpret_cast<Function>(loader(name));
            return function != nullptr;
        }

        struct OpenGLFunctions
        {
            using GenVertexArrays = void (*)(GLsizei, GLuint*);
            using BindVertexArray = void (*)(GLuint);
            using DeleteVertexArrays = void (*)(GLsizei, const GLuint*);
            using GenBuffers = void (*)(GLsizei, GLuint*);
            using BindBuffer = void (*)(GLenum, GLuint);
            using BufferData = void (*)(GLenum, GLsizeiptr, const void*, GLenum);
            using DeleteBuffers = void (*)(GLsizei, const GLuint*);
            using EnableVertexAttribArray = void (*)(GLuint);
            using VertexAttribPointer = void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
            using CreateShader = GLuint (*)(GLenum);
            using ShaderSource = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
            using CompileShader = void (*)(GLuint);
            using GetShaderiv = void (*)(GLuint, GLenum, GLint*);
            using DeleteShader = void (*)(GLuint);
            using CreateProgram = GLuint (*)(void);
            using AttachShader = void (*)(GLuint, GLuint);
            using BindAttribLocation = void (*)(GLuint, GLuint, const GLchar*);
            using LinkProgram = void (*)(GLuint);
            using GetProgramiv = void (*)(GLuint, GLenum, GLint*);
            using DeleteProgram = void (*)(GLuint);
            using UseProgram = void (*)(GLuint);
            using GetUniformLocation = GLint (*)(GLuint, const GLchar*);
            using Uniform2f = void (*)(GLint, GLfloat, GLfloat);
            using Uniform4f = void (*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
            using DrawArrays = void (*)(GLenum, GLint, GLsizei);
            using ClearColor = void (*)(GLfloat, GLfloat, GLfloat, GLfloat);
            using Clear = void (*)(GLbitfield);
            using Viewport = void (*)(GLint, GLint, GLsizei, GLsizei);
            using Enable = void (*)(GLenum);
            using Scissor = void (*)(GLint, GLint, GLsizei, GLsizei);

            GenVertexArrays gen_vertex_arrays{};
            BindVertexArray bind_vertex_array{};
            DeleteVertexArrays delete_vertex_arrays{};
            GenBuffers gen_buffers{};
            BindBuffer bind_buffer{};
            BufferData buffer_data{};
            DeleteBuffers delete_buffers{};
            EnableVertexAttribArray enable_vertex_attrib_array{};
            VertexAttribPointer vertex_attrib_pointer{};
            CreateShader create_shader{};
            ShaderSource shader_source{};
            CompileShader compile_shader{};
            GetShaderiv get_shader_iv{};
            DeleteShader delete_shader{};
            CreateProgram create_program{};
            AttachShader attach_shader{};
            BindAttribLocation bind_attrib_location{};
            LinkProgram link_program{};
            GetProgramiv get_program_iv{};
            DeleteProgram delete_program{};
            UseProgram use_program{};
            GetUniformLocation get_uniform_location{};
            Uniform2f uniform_2f{};
            Uniform4f uniform_4f{};
            DrawArrays draw_arrays{};
            ClearColor clear_color{};
            Clear clear{};
            Viewport viewport{};
            Enable enable{};
            Enable disable{};
            Scissor scissor{};

            [[nodiscard]] bool load(OpenGLProcLoader loader) noexcept
            {
                if (!loader)
                    return false;
                return load_function(gen_vertex_arrays, loader, "glGenVertexArrays")
                    && load_function(bind_vertex_array, loader, "glBindVertexArray")
                    && load_function(delete_vertex_arrays, loader, "glDeleteVertexArrays")
                    && load_function(gen_buffers, loader, "glGenBuffers")
                    && load_function(bind_buffer, loader, "glBindBuffer")
                    && load_function(buffer_data, loader, "glBufferData")
                    && load_function(delete_buffers, loader, "glDeleteBuffers")
                    && load_function(enable_vertex_attrib_array, loader, "glEnableVertexAttribArray")
                    && load_function(vertex_attrib_pointer, loader, "glVertexAttribPointer")
                    && load_function(create_shader, loader, "glCreateShader")
                    && load_function(shader_source, loader, "glShaderSource")
                    && load_function(compile_shader, loader, "glCompileShader")
                    && load_function(get_shader_iv, loader, "glGetShaderiv")
                    && load_function(delete_shader, loader, "glDeleteShader")
                    && load_function(create_program, loader, "glCreateProgram")
                    && load_function(attach_shader, loader, "glAttachShader")
                    && load_function(bind_attrib_location, loader, "glBindAttribLocation")
                    && load_function(link_program, loader, "glLinkProgram")
                    && load_function(get_program_iv, loader, "glGetProgramiv")
                    && load_function(delete_program, loader, "glDeleteProgram")
                    && load_function(use_program, loader, "glUseProgram")
                    && load_function(get_uniform_location, loader, "glGetUniformLocation")
                    && load_function(uniform_2f, loader, "glUniform2f")
                    && load_function(uniform_4f, loader, "glUniform4f")
                    && load_function(draw_arrays, loader, "glDrawArrays")
                    && load_function(clear_color, loader, "glClearColor")
                    && load_function(clear, loader, "glClear")
                    && load_function(viewport, loader, "glViewport")
                    && load_function(enable, loader, "glEnable")
                    && load_function(disable, loader, "glDisable")
                    && load_function(scissor, loader, "glScissor");
            }
        };

        struct DrawCommand
        {
            GLint first{};
            GLsizei count{};
            Color color{};
            std::optional<gui::Rect> clip{};
        };

        struct SceneBatch
        {
            std::vector<gui::Vec2> vertices{};
            std::vector<DrawCommand> commands{};
            gui::Vec2 translation{};
            gui::Vec2 pixel_scale{1.0f, 1.0f};
            std::optional<gui::Rect> clip{};

            void clear()
            {
                vertices.clear();
                commands.clear();
                translation = {};
                clip.reset();
            }

            void append_triangle(gui::Vec2 a, gui::Vec2 b, gui::Vec2 c)
            {
                for (auto point : {a, b, c})
                    vertices.push_back({point.x + translation.x, point.y + translation.y});
            }

            void append_rect(gui::Rect rect, Color color)
            {
                if (rect.size.x <= 0.0f || rect.size.y <= 0.0f)
                    return;
                const GLint first = static_cast<GLint>(vertices.size());
                const float left = rect.position.x;
                const float top = rect.position.y;
                const float right = left + rect.size.x;
                const float bottom = top + rect.size.y;
                append_triangle({ left, top }, { right, top }, { right, bottom });
                append_triangle({ left, top }, { right, bottom }, { left, bottom });
                commands.push_back({ first, 6, color, clip });
            }

            void append_glyph_pixel(gui::Rect rect, Color color)
            {
                // Bitmap cells must cover whole framebuffer pixels. Fractional
                // quads at the small label scales are unstable under viewport
                // scaling; clipping geometry and ordinary surfaces stay unchanged.
                const float left = std::round((rect.position.x + translation.x) * pixel_scale.x);
                const float top = std::round((rect.position.y + translation.y) * pixel_scale.y);
                const float right = (std::max)(left + 1.0f,
                    std::round((rect.position.x + translation.x + rect.size.x) * pixel_scale.x));
                const float bottom = (std::max)(top + 1.0f,
                    std::round((rect.position.y + translation.y + rect.size.y) * pixel_scale.y));
                append_rect({{left / pixel_scale.x - translation.x, top / pixel_scale.y - translation.y},
                    {(right - left) / pixel_scale.x, (bottom - top) / pixel_scale.y}}, color);
            }

            void append_indexed(
                const rounded::RoundedRectMesh& mesh,
                std::span<const std::uint32_t> indices,
                Color color)
            {
                if (indices.empty())
                    return;
                const GLint first = static_cast<GLint>(vertices.size());
                for (const std::uint32_t index : indices)
                {
                    if (index < mesh.vertices.size())
                        vertices.push_back({mesh.vertices[index].x + translation.x,
                                            mesh.vertices[index].y + translation.y});
                }
                const GLsizei count = static_cast<GLsizei>(vertices.size()) - first;
                if (count > 0)
                    commands.push_back({ first, count, color, clip });
            }
        };

        [[nodiscard]] gui::Rect intersection(gui::Rect a, gui::Rect b) noexcept
        {
            const float x = (std::max)(a.position.x, b.position.x);
            const float y = (std::max)(a.position.y, b.position.y);
            return {{x, y}, {(std::max)(0.0f, (std::min)(a.position.x + a.size.x, b.position.x + b.size.x) - x),
                             (std::max)(0.0f, (std::min)(a.position.y + a.size.y, b.position.y + b.size.y) - y)}};
        }

        struct ClipScope
        {
            SceneBatch& batch;
            std::optional<gui::Rect> previous;
            ClipScope(SceneBatch& target, gui::Rect local) : batch(target), previous(target.clip)
            {
                local.position.x += target.translation.x;
                local.position.y += target.translation.y;
                target.clip = previous ? intersection(*previous, local) : local;
            }
            ~ClipScope() { batch.clip = previous; }
        };

        [[nodiscard]] gui::Rect inset(gui::Rect rect, float amount) noexcept
        {
            rect.position.x += amount;
            rect.position.y += amount;
            rect.size.x = (std::max)(0.0f, rect.size.x - amount * 2.0f);
            rect.size.y = (std::max)(0.0f, rect.size.y - amount * 2.0f);
            return rect;
        }

        void draw_surface(
            SceneBatch& batch,
            gui::Rect rect,
            bool rounded_surface,
            float radius,
            Color fill,
            Color border = panel_border,
            float border_width = 1.0f)
        {
            if (!rounded_surface)
            {
                if (border_width > 0.0f)
                {
                    batch.append_rect(rect, border);
                    batch.append_rect(inset(rect, border_width), fill);
                }
                else
                {
                    batch.append_rect(rect, fill);
                }
                return;
            }

            const rounded::RoundedRectMesh mesh = rounded::make_rounded_rect_mesh({
                .bounds = rect,
                .radii = { radius, radius, radius, radius },
                .border_width = border_width,
                .segments_per_corner = 10
            });
            if (!mesh.valid)
                return;
            batch.append_indexed(mesh, mesh.fill_indices, fill);
            batch.append_indexed(mesh, mesh.border_indices, border);
        }

        void draw_text(
            SceneBatch& batch,
            float x,
            float y,
            std::string_view text,
            float scale,
            Color color,
            float maximum_width = 10000.0f)
        {
            float cursor_x = x;
            float cursor_y = y;
            for (const char character : text)
            {
                if (character == '\n')
                {
                    cursor_x = x;
                    cursor_y += scale * static_cast<float>(font::line_advance);
                    continue;
                }
                if (cursor_x + scale * static_cast<float>(font::glyph_width) > x + maximum_width)
                    continue;
                if (character != ' ')
                {
                    const font::BitmapGlyph glyph = font::default_glyph(character);
                    for (std::uint32_t row = 0; row < font::glyph_height; ++row)
                    {
                        for (std::uint32_t column = 0; column < font::glyph_width; ++column)
                        {
                            if (font::pixel_on(glyph, column, row))
                            {
                                batch.append_glyph_pixel({
                                    {
                                        cursor_x + static_cast<float>(column) * scale,
                                        cursor_y + static_cast<float>(row) * scale
                                    },
                                    { scale, scale }
                                }, color);
                            }
                        }
                    }
                }
                cursor_x += scale * static_cast<float>(font::glyph_advance);
            }
        }

        [[nodiscard]] image::Image load_demo_image()
        {
            constexpr std::array<std::string_view, 5> paths{
                "assets/epochgui_demo.ppm",
                "../assets/epochgui_demo.ppm",
                "../../assets/epochgui_demo.ppm",
                "../../../assets/epochgui_demo.ppm",
                "../../../../assets/epochgui_demo.ppm"
            };
            for (const std::string_view path : paths)
            {
                image::ImageResult result = image::load_ppm_file(path);
                if (result)
                    return std::move(result.image);
            }
            return {};
        }

        [[nodiscard]] constexpr Color color_from_rgba8(image::Rgba8 color) noexcept
        {
            constexpr float inverse_255 = 1.0f / 255.0f;
            return {
                static_cast<float>(color.r) * inverse_255,
                static_cast<float>(color.g) * inverse_255,
                static_cast<float>(color.b) * inverse_255,
                static_cast<float>(color.a) * inverse_255
            };
        }

        [[nodiscard]] constexpr std::string_view ppm_format_label(const image::Image& raster) noexcept
        {
            return raster.encoding == image::PpmEncoding::binary_p6
                ? "FORMAT P6 PPM"
                : "FORMAT P3 PPM";
        }

        void draw_image(SceneBatch& batch, const image::Image& raster, gui::Rect viewport, bool fit)
        {
            draw_surface(batch, viewport, true, 10.0f, background, panel_border, 1.0f);
            if (!raster.valid())
                return;

            const image::RasterImageLayout layout = image::make_raster_image_layout(
                raster,
                viewport,
                fit ? image::ImageFit::contain : image::ImageFit::stretch,
                12.0f);
            if (!layout.valid)
                return;

            for (std::uint32_t y = 0; y < raster.height; ++y)
            {
                for (std::uint32_t x = 0; x < raster.width; ++x)
                {
                    const image::Rgba8* pixel = raster.pixel(x, y);
                    if (!pixel)
                        continue;
                    batch.append_rect(
                        image::raster_pixel_rect(layout, x, y, 0.25f),
                        color_from_rgba8(*pixel));
                }
            }
        }

        [[nodiscard]] GLuint compile_shader(OpenGLFunctions& gl, GLenum type, const char* source) noexcept
        {
            const GLuint shader = gl.create_shader(type);
            if (shader == 0)
                return 0;
            gl.shader_source(shader, 1, &source, nullptr);
            gl.compile_shader(shader);
            GLint compiled{};
            gl.get_shader_iv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == 0)
            {
                gl.delete_shader(shader);
                return 0;
            }
            return shader;
        }

        [[nodiscard]] GLuint create_program(OpenGLFunctions& gl) noexcept
        {
            constexpr const char* vertex_source = R"(#version 150 core
in vec2 inPosition;
uniform vec2 uCanvas;
void main()
{
    vec2 clip = vec2((inPosition.x / uCanvas.x) * 2.0 - 1.0,
                     1.0 - (inPosition.y / uCanvas.y) * 2.0);
    gl_Position = vec4(clip, 0.0, 1.0);
})";
            constexpr const char* fragment_source = R"(#version 150 core
uniform vec4 uColor;
out vec4 outColor;
void main()
{
    outColor = uColor;
})";

            const GLuint vertex_shader = compile_shader(gl, GL_VERTEX_SHADER, vertex_source);
            const GLuint fragment_shader = compile_shader(gl, GL_FRAGMENT_SHADER, fragment_source);
            if (vertex_shader == 0 || fragment_shader == 0)
            {
                if (vertex_shader != 0)
                    gl.delete_shader(vertex_shader);
                if (fragment_shader != 0)
                    gl.delete_shader(fragment_shader);
                return 0;
            }

            const GLuint program = gl.create_program();
            if (program == 0)
            {
                gl.delete_shader(vertex_shader);
                gl.delete_shader(fragment_shader);
                return 0;
            }
            gl.attach_shader(program, vertex_shader);
            gl.attach_shader(program, fragment_shader);
            gl.bind_attrib_location(program, 0, "inPosition");
            gl.link_program(program);
            gl.delete_shader(fragment_shader);
            gl.delete_shader(vertex_shader);
            GLint linked{};
            gl.get_program_iv(program, GL_LINK_STATUS, &linked);
            if (linked == 0)
            {
                gl.delete_program(program);
                return 0;
            }
            return program;
        }
    }

    struct OpenGLRenderer::Impl
    {
        LibraryModels libraries{};
        OpenGLFunctions gl{};
        SceneBatch batch{};
        input::InputTracker input_tracker{};
        input::BorderlessWindowChromeLayout chrome_layout{};
        input::WindowCommand pending_window_command{ input::WindowCommand::none };
        int pending_native_frame_mode{ -1 };
        bool native_frame_enabled{ true };

        gui::FloatingWindowState core_window{};
        gui::FloatingWindowState rounded_window{};
        gui::FloatingWindowLayout core_layout{};
        gui::FloatingWindowLayout rounded_layout{};
        gui::PopupState context_popup{};
        gui::PopupLayout context_layout{};
        gui::Vec2 context_anchor{};
        bool context_text{};

        gui::DockLayoutState dock_state{};
        std::array<gui::DockPaneState, 3> dock_panes{};
        gui::DockableWindowHostState dockable_host{};
        gui::DockableWindowState dockable_state{};
        gui::DockableWindowResult dockable_result{};
        gui::PanelHostState panel_host_state{};
        gui::PanelHostResult panel_host_result{};

        gui::TextControlState edit_state{};
        gui::TextControlState multiline_state{};
        gui::TextControlState static_state{};
        std::string pending_text{};
        std::uint32_t active_text_control{};
        int pending_workspace_step{};

        image::Image demo_image{};
        bool image_fit{ true };
        float splitter_fraction{ 0.37f };
        std::uint32_t selected_row{ 1 };
        std::uint32_t page{EPOCHGUI_DEMO_LIBRARY_PAGE ? 5u : 0u};
        std::uint32_t display_mode{ 2 };
        std::uint32_t next_focus_order{ 3 };
        std::array<bool, 4> showcase_open{true, true, true, false};
        float showcase_scroll{};
        bool showcase_scroll_drag{};
        float showcase_scroll_grab{};
        bool text_drag{};
        std::string demo_clipboard{};
        const char* (*clipboard_read)(void*){};
        bool (*clipboard_write)(void*, const char*){};
        void* clipboard_user{};
        std::string edit_status{"WHEEL SCROLLS NOTES. TEXT AND CARET STAY IN THEIR FIELD."};
        float demo_value{0.68f};
        bool show_tips{true};

        GLuint program{};
        GLuint vertex_array{};
        GLuint vertex_buffer{};
        GLint canvas_uniform{ -1 };
        GLint color_uniform{ -1 };
        int framebuffer_width{ static_cast<int>(canvas_width) };
        int framebuffer_height{ static_cast<int>(canvas_height) };
        bool initialized{};

        Impl()
        {
            reset_windows();
            rebuild_chrome();
            reset_docking();
            edit_state.text = "A SMALL PROJECT";
            multiline_state.text = "Write a note here. Long lines wrap inside this editor.\n\nDrag to select, use Shift + arrows, or try the editing buttons below.\n\nThe mouse wheel scrolls this field without moving the caret.\n\nPaste several paragraphs to test clipping and scroll limits.";
            static_state.text = "READ ONLY\n\nThis uses the same EpochGui text controller as the editable field.\n\nText, selection and the caret stay inside their own viewport.\n\nThe showcase uses the public library APIs; it does not need EpochEngine.";
            demo_image = load_demo_image();
        }

        ~Impl()
        {
            release();
        }

        void reset_windows() noexcept
        {
            core_window = {
                .position = { 320.0f, 250.0f },
                .size = { 390.0f, 250.0f },
                .open = true,
                .initialized = true,
                .focus_order = 1
            };
            rounded_window = {
                .position = { 690.0f, 360.0f },
                .size = { 410.0f, 272.0f },
                .open = true,
                .initialized = true,
                .focus_order = 2
            };
            next_focus_order = 3;
        }

        void reset_docking() noexcept
        {
            dock_state = {};
            dock_panes = {
                gui::DockPaneState{ .id = 1, .slot = gui::DockSlot::left, .min_size = { 120.0f, 90.0f }, .weight = 1.0f, .visible = true },
                gui::DockPaneState{ .id = 2, .slot = gui::DockSlot::center, .min_size = { 200.0f, 120.0f }, .weight = 2.0f, .visible = true },
                gui::DockPaneState{ .id = 3, .slot = gui::DockSlot::right, .min_size = { 120.0f, 90.0f }, .weight = 1.0f, .visible = true }
            };
            dockable_host = {};
            dockable_state = {
                .id = 10,
                .mode = gui::DockableWindowMode::docked,
                .dock_slot = gui::DockSlot::right,
                .visible = true
            };
            panel_host_state = {
                .id = 20,
                .mode = gui::PanelHostMode::docked,
                .dock_slot = gui::DockSlot::bottom,
                .visible = true
            };
        }

        void rebuild_chrome() noexcept
        {
            chrome_layout = input::make_borderless_window_chrome_layout({
                .bounds = { { 0.0f, 0.0f }, { canvas_width, canvas_height } },
                .title_bar_height = custom_title_height,
                .resize_border = 7.0f,
                .caption_padding_left = 18.0f,
                .button_width = 48.0f,
                .movable = true,
                .resizable = true,
                .minimizable = true,
                .maximizable = true,
                .closable = true
            });
        }

        [[nodiscard]] gui::Vec2 to_canvas(float x, float y) const noexcept
        {
            const float width = framebuffer_width > 0 ? static_cast<float>(framebuffer_width) : canvas_width;
            const float height = framebuffer_height > 0 ? static_cast<float>(framebuffer_height) : canvas_height;
            return { x * canvas_width / width, y * canvas_height / height };
        }

        void release() noexcept
        {
            if (vertex_buffer != 0)
                gl.delete_buffers(1, &vertex_buffer);
            if (vertex_array != 0)
                gl.delete_vertex_arrays(1, &vertex_array);
            if (program != 0)
                gl.delete_program(program);
            vertex_buffer = 0;
            vertex_array = 0;
            program = 0;
            initialized = false;
        }

        [[nodiscard]] bool initialize(OpenGLProcLoader loader) noexcept
        {
            if (initialized)
                return true;
            if (!gl.load(loader))
                return false;
            program = create_program(gl);
            if (program == 0)
                return false;
            canvas_uniform = gl.get_uniform_location(program, "uCanvas");
            color_uniform = gl.get_uniform_location(program, "uColor");
            if (canvas_uniform < 0 || color_uniform < 0)
            {
                release();
                return false;
            }
            gl.gen_vertex_arrays(1, &vertex_array);
            gl.bind_vertex_array(vertex_array);
            gl.gen_buffers(1, &vertex_buffer);
            gl.bind_buffer(GL_ARRAY_BUFFER, vertex_buffer);
            gl.enable_vertex_attrib_array(0);
            gl.vertex_attrib_pointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(gui::Vec2), nullptr);
            initialized = vertex_array != 0 && vertex_buffer != 0;
            if (!initialized)
                release();
            return initialized;
        }

        [[nodiscard]] gui::Rect content_area() const noexcept
        {
            return { { sidebar_width, workspace_top }, { canvas_width - sidebar_width, canvas_height - workspace_top } };
        }

        [[nodiscard]] gui::FloatingWindowOptions floating_options(gui::Vec2 position, gui::Vec2 size) const noexcept
        {
            return {
                .default_position = position,
                .default_size = size,
                .min_size = { 270.0f, 190.0f },
                .viewport_size = { canvas_width, canvas_height },
                .title_bar_height = 32.0f,
                .content_padding = 10.0f,
                .movable = true,
                .resizable = true,
                .closable = true
            };
        }

        void clamp_to_workspace(gui::FloatingWindowState& state) noexcept
        {
            state.position.x = (std::max)(sidebar_width + 8.0f, state.position.x);
            state.position.y = (std::max)(workspace_top + 8.0f, state.position.y);
        }

        [[nodiscard]] int press_target(const input::InputFrame& frame) const noexcept
        {
            if (!frame.pointer(input::PointerButton::left).pressed)
                return 0;
            if (context_layout.visible && gui::contains(context_layout.popup, frame.pointer_position))
                return 0;
            const bool core_visible = core_window.open && display_mode != 1;
            const bool rounded_visible = rounded_window.open && display_mode != 0;
            const bool in_core = core_visible && gui::contains({ core_window.position, core_window.size }, frame.pointer_position);
            const bool in_rounded = rounded_visible && gui::contains({ rounded_window.position, rounded_window.size }, frame.pointer_position);
            if (in_core && in_rounded)
                return core_window.focus_order > rounded_window.focus_order ? 1 : 2;
            if (in_core)
                return 1;
            if (in_rounded)
                return 2;
            return 0;
        }

        void update_floating_windows(const input::InputFrame& frame) noexcept
        {
            const int target = press_target(frame);
            const gui::FloatingWindowInput actual = input::floating_window_input(frame);
            gui::FloatingWindowInput core_input = actual;
            gui::FloatingWindowInput rounded_input = actual;

            const bool core_captured = core_window.dragging
                || core_window.resizing
                || core_window.close_pressed
                || target == 1;
            const bool rounded_captured = rounded_window.dragging
                || rounded_window.resizing
                || rounded_window.close_pressed
                || target == 2;
            if (!core_captured)
                core_input = { .mouse_position = actual.mouse_position };
            if (!rounded_captured)
                rounded_input = { .mouse_position = actual.mouse_position };

            clamp_to_workspace(core_window);
            clamp_to_workspace(rounded_window);
            if (display_mode != 1)
            {
                core_layout = gui::update_floating_window(
                    core_window,
                    floating_options({ 320.0f, 250.0f }, { 390.0f, 250.0f }),
                    core_input);
                clamp_to_workspace(core_window);
                if (core_layout.focused)
                    core_window.focus_order = next_focus_order++;
            }
            else
            {
                core_layout = {};
            }

            if (display_mode != 0)
            {
                rounded_layout = gui::update_floating_window(
                    rounded_window,
                    floating_options({ 690.0f, 360.0f }, { 410.0f, 272.0f }),
                    rounded_input);
                clamp_to_workspace(rounded_window);
                if (rounded_layout.focused)
                    rounded_window.focus_order = next_focus_order++;
            }
            else
            {
                rounded_layout = {};
            }
        }

        void update_navigation(const input::InputFrame& frame) noexcept
        {
            constexpr std::array<float, 3> widths{ 150.0f, 174.0f, 160.0f };
            constexpr std::array<std::uint32_t, 3> pages{0, 1, 5};
            const gui::SegmentedControlLayoutOptions options{
                .position = { 22.0f, 67.0f },
                .item_widths = widths,
                .height = 32.0f,
                .gap = 4.0f
            };
            if (frame.pointer(input::PointerButton::left).released)
            {
                const std::uint32_t item = gui::segmented_control_item_at(options, frame.pointer_position);
                if (item < pages.size())
                {
                    page = pages[item];
                    active_text_control = 0;
                    edit_state.focused = multiline_state.focused = static_state.focused = false;
                }
                for (std::size_t index = 0; index < showcase_open.size(); ++index)
                {
                    if (!gui::contains({{14.0f, 180.0f + static_cast<float>(index) * 52.0f}, {186.0f, 42.0f}}, frame.pointer_position))
                        continue;
                    page = 0;
                    showcase_open[index] = true;
                    showcase_scroll += section_header(index).position.y - showcase_viewport().position.y;
                    showcase_scroll = (std::max)(0.0f, showcase_scroll);
                }
            }
        }

        [[nodiscard]] static gui::Rect showcase_viewport() noexcept
        {
            return {{234.0f, 184.0f}, {1008.0f, 612.0f}};
        }

        [[nodiscard]] gui::Rect section_header(std::size_t index) const noexcept
        {
            float y = showcase_viewport().position.y - showcase_scroll;
            for (std::size_t previous = 0; previous < index; ++previous)
                y += showcase_open[previous] ? 656.0f : 48.0f;
            return {{246.0f, y}, {984.0f, 34.0f}};
        }

        [[nodiscard]] float showcase_extent() const noexcept
        {
            float extent{};
            for (const bool open : showcase_open) extent += open ? 656.0f : 48.0f;
            return extent;
        }

        [[nodiscard]] gui::ScrollbarLayout showcase_scrollbar() const noexcept
        {
            return gui::make_scrollbar_layout({
                .track = {{1250.0f, showcase_viewport().position.y}, {12.0f, showcase_viewport().size.y}},
                .content_extent = showcase_extent(), .scroll_offset = showcase_scroll,
                .minimum_thumb_extent = 36.0f});
        }

        [[nodiscard]] input::InputFrame section_input(const input::InputFrame& frame,
            std::size_t section) const noexcept
        {
            auto local = frame;
            local.pointer_position.y -= section_header(section).position.y + 44.0f - 184.0f;
            if (!gui::contains(showcase_viewport(), frame.pointer_position))
            {
                local.pointer_buttons = {};
                local.wheel_delta = {};
                local.pointer_position = {-10000.0f, -10000.0f};
            }
            return local;
        }

        void update_showcase(const input::InputFrame& frame)
        {
            const auto& left = frame.pointer(input::PointerButton::left);
            auto scroll = showcase_scrollbar();
            if (left.pressed && gui::contains(scroll.track, frame.pointer_position))
            {
                showcase_scroll_drag = true;
                showcase_scroll_grab = gui::scrollbar_grab_offset(scroll, frame.pointer_position.y,
                    gui::contains(scroll.thumb, frame.pointer_position));
            }
            if (showcase_scroll_drag && left.down)
                showcase_scroll = gui::scrollbar_offset_from_pointer(scroll, frame.pointer_position.y, showcase_scroll_grab);
            if (!left.down) showcase_scroll_drag = false;
            if (left.released && gui::contains(showcase_viewport(), frame.pointer_position))
                for (std::size_t index = 0; index < showcase_open.size(); ++index)
                    if (gui::contains(section_header(index), frame.pointer_position))
                    {
                        showcase_open[index] = !showcase_open[index];
                        if (index == 1 && !showcase_open[index])
                        {
                            active_text_control = 0;
                            edit_state.focused = multiline_state.focused = static_state.focused = false;
                        }
                        break;
                    }
            const auto text_frame = section_input(frame, 1);
            const bool field_wheel = showcase_open[1]
                && (gui::contains({{270.0f, 316.0f}, {430.0f, 190.0f}}, text_frame.pointer_position)
                    || gui::contains({{744.0f, 210.0f}, {440.0f, 296.0f}}, text_frame.pointer_position));
            if (gui::contains(showcase_viewport(), frame.pointer_position) && !field_wheel)
                showcase_scroll -= frame.wheel_delta.y * 46.0f;
            scroll = showcase_scrollbar();
            showcase_scroll = (std::clamp)(showcase_scroll, 0.0f, scroll.maximum_scroll_offset);
            if (showcase_open[0]) update_overview(section_input(frame, 0));
            if (showcase_open[1]) update_text_page(section_input(frame, 1));
            else pending_text.clear();
            if (showcase_open[2]) update_image_page(section_input(frame, 2));
        }

        [[nodiscard]] gui::Rect frame_toggle_rect() const noexcept
        {
            return { { 1000.0f, 67.0f }, { 250.0f, 32.0f } };
        }

        void update_frame_toggle(const input::InputFrame& frame) noexcept
        {
            if (EPOCHGUI_DEMO_PLATFORM_HOSTED) return;
            if (frame.pointer(input::PointerButton::left).released
                && gui::contains(frame_toggle_rect(), frame.pointer_position))
            {
                native_frame_enabled = !native_frame_enabled;
                pending_native_frame_mode = native_frame_enabled ? 1 : 0;
            }
        }

        [[nodiscard]] static gui::SegmentedControlLayoutOptions window_mode_options() noexcept
        {
            static constexpr std::array<float, 3> widths{92.0f, 102.0f, 92.0f};
            return {.position = {246.0f, 164.0f}, .item_widths = widths, .height = 32.0f, .gap = 4.0f};
        }

        [[nodiscard]] static gui::DockLayoutOptions window_dock_options() noexcept
        {
            return {.workspace = {{246.0f, 214.0f}, {970.0f, 176.0f}},
                .min_pane_size = {96.0f, 64.0f}, .default_popout_size = {300.0f, 190.0f},
                .edge_fraction = 0.22f, .title_bar_height = 28.0f, .content_padding = 6.0f,
                .popout_spacing = 22.0f, .visible_margin = 48.0f};
        }

        void update_display_mode(const input::InputFrame& frame) noexcept
        {
            const auto options = window_mode_options();
            if (frame.pointer(input::PointerButton::left).released)
            {
                const std::uint32_t item = gui::segmented_control_item_at(options, frame.pointer_position);
                if (item != gui::invalid_selectable_row_index)
                    display_mode = item;
            }
        }

        void update_overview(const input::InputFrame& frame) noexcept
        {
            const gui::SplitterLayoutOptions options{
                .area = { { 250.0f, 610.0f }, { 940.0f, 142.0f } },
                .axis = gui::SplitterAxis::vertical,
                .split_fraction = splitter_fraction,
                .thickness = 8.0f,
                .min_before = 180.0f,
                .min_after = 260.0f
            };
            const gui::SplitterLayout split = gui::make_splitter_layout(options);
            if (frame.pointer(input::PointerButton::left).down
                && gui::splitter_hit_test(split, frame.pointer_position, 5.0f))
            {
                splitter_fraction = gui::splitter_fraction_from_point(options, frame.pointer_position);
            }
            const gui::SliderLayoutOptions value_options{
                .bounds = {{620.0f, 398.0f}, {270.0f, 28.0f}}, .value = demo_value,
                .thumb_width = 14.0f, .thumb_height = 20.0f};
            if (frame.pointer(input::PointerButton::left).down
                && gui::contains(value_options.bounds, frame.pointer_position))
                demo_value = gui::slider_value_from_position(value_options, frame.pointer_position.x);
            if (frame.pointer(input::PointerButton::left).released
                && gui::contains({{940.0f, 398.0f}, {52.0f, 26.0f}}, frame.pointer_position))
                show_tips = !show_tips;

            const gui::SelectableListLayoutOptions list_options{
                .viewport = { { 270.0f, 278.0f }, { 280.0f, 190.0f } },
                .row_count = 5,
                .row_height = 30.0f,
                .row_gap = 4.0f,
                .content_padding_x = 8.0f,
                .content_padding_y = 8.0f
            };
            if (frame.pointer(input::PointerButton::left).released)
            {
                const std::uint32_t row = gui::selectable_row_index_at(list_options, frame.pointer_position);
                if (row != gui::invalid_selectable_row_index)
                    selected_row = row;
            }
        }

        void update_windows_page(const input::InputFrame& frame) noexcept
        {
            update_display_mode(frame);
            update_floating_windows(frame);

            const auto options = window_dock_options();
            gui::DockLayoutInput dock_input{
                .mouse_position = frame.pointer_position,
                .mouse_pressed = frame.pointer(input::PointerButton::left).pressed,
                .mouse_released = frame.pointer(input::PointerButton::left).released
            };
            if (frame.pointer(input::PointerButton::left).released)
            {
                for (const gui::DockPaneState& pane : dock_panes)
                {
                    const gui::DockPaneLayout layout = gui::make_dock_pane_layout(pane, options, dock_input);
                    if (gui::contains(layout.title_bar, frame.pointer_position))
                    {
                        dock_input.toggle_popout_pane_id = pane.id;
                        break;
                    }
                }
            }
            (void)gui::update_dock_layout(dock_state, dock_panes.data(), static_cast<std::uint32_t>(dock_panes.size()), options, dock_input);

            const gui::Rect dock_button{ { 246.0f, 704.0f }, { 142.0f, 30.0f } };
            const gui::Rect float_button{ { 396.0f, 704.0f }, { 142.0f, 30.0f } };
            const gui::Rect detach_button{ { 546.0f, 704.0f }, { 142.0f, 30.0f } };
            gui::DockableWindowAction action = gui::DockableWindowAction::none;
            if (frame.pointer(input::PointerButton::left).released)
            {
                if (gui::contains(dock_button, frame.pointer_position)) action = gui::DockableWindowAction::dock;
                else if (gui::contains(float_button, frame.pointer_position)) action = gui::DockableWindowAction::float_window;
                else if (gui::contains(detach_button, frame.pointer_position)) action = gui::DockableWindowAction::detach;
            }
            dockable_result = gui::update_dockable_window(
                dockable_host,
                dockable_state,
                {
                    .title = "DOCKABLE",
                    .docked_frame = { { 720.0f, 590.0f }, { 230.0f, 142.0f } },
                    .floating = floating_options({ 720.0f, 590.0f }, { 230.0f, 142.0f }),
                    .viewport_size = { canvas_width, canvas_height },
                    .title_bar_height = 28.0f,
                    .content_padding = 6.0f,
                    .action_button_width = 36.0f,
                    .action_button_gap = 4.0f,
                    .allow_dock = true,
                    .allow_float = true,
                    .allow_detach = true,
                    .allow_close = true,
                    .fallback_dock_slot = gui::DockSlot::right
                },
                {
                    .mouse_position = frame.pointer_position,
                    .mouse_down = frame.pointer(input::PointerButton::left).down,
                    .mouse_pressed = frame.pointer(input::PointerButton::left).pressed,
                    .mouse_released = frame.pointer(input::PointerButton::left).released,
                    .requested_action = action
                });

            gui::PanelHostAction panel_action = gui::PanelHostAction::none;
            const gui::Rect panel_button{ { 970.0f, 704.0f }, { 220.0f, 30.0f } };
            if (frame.pointer(input::PointerButton::left).released
                && gui::contains(panel_button, frame.pointer_position))
            {
                panel_action = panel_host_state.mode == gui::PanelHostMode::docked
                    ? gui::PanelHostAction::float_panel
                    : gui::PanelHostAction::dock;
            }
            panel_host_result = gui::update_panel_host(
                panel_host_state,
                {
                    .title = "PANEL HOST",
                    .docked_frame = { { 970.0f, 590.0f }, { 220.0f, 104.0f } },
                    .floating = floating_options({ 850.0f, 540.0f }, { 250.0f, 160.0f }),
                    .popup = {
                        .owner = panel_button,
                        .preferred_size = { 250.0f, 160.0f },
                        .viewport_size = { canvas_width, canvas_height }
                    },
                    .default_external_frame = { { 100.0f, 100.0f }, { 500.0f, 360.0f } },
                    .fallback_dock_slot = gui::DockSlot::bottom,
                    .allow_dock = true,
                    .allow_float = true,
                    .allow_popup = true,
                    .allow_external_host = true,
                    .allow_close = true
                },
                {
                    .focus_host = &dockable_host,
                    .mouse_position = frame.pointer_position,
                    .mouse_down = frame.pointer(input::PointerButton::left).down,
                    .mouse_pressed = frame.pointer(input::PointerButton::left).pressed,
                    .mouse_released = frame.pointer(input::PointerButton::left).released,
                    .requested_action = panel_action,
                    .escape_pressed = frame.key(input::Key::escape).pressed
                });
        }

        [[nodiscard]] static gui::Rect text_rect(std::uint32_t id) noexcept
        {
            if (id == 1) return {{270.0f, 210.0f}, {430.0f, 48.0f}};
            if (id == 2) return {{270.0f, 316.0f}, {430.0f, 190.0f}};
            return {{744.0f, 210.0f}, {440.0f, 296.0f}};
        }

        [[nodiscard]] static gui::TextControlOptions text_options(std::uint32_t id) noexcept
        {
            return {.viewport_size = text_rect(id).size, .content_padding = {10.0f, 10.0f},
                .maximum_bytes = id == 1 ? 256u : 65536u, .multiline = id != 1,
                .read_only = id == 3, .accept_tab = id == 2};
        }

        [[nodiscard]] gui::TextControlState* focused_text() noexcept
        {
            return active_text_control == 1 ? &edit_state : active_text_control == 2
                ? &multiline_state : active_text_control == 3 ? &static_state : nullptr;
        }

        void apply_text_command(gui::TextControlState& state, const gui::TextControlOptions& options,
            gui::TextControlCommand command, std::string_view text = {},
            std::size_t caret = gui::invalid_text_index, bool extend = false)
        {
            const auto before = text_view::layout(state, options.viewport_size, 1.6f, options.multiline);
            const auto result = gui::update_text_control(state, options, {
                .command = command, .text = text, .requested_caret = caret,
                .metrics = before.metrics, .extend_selection = extend});
            if (result.clipboard_write_requested) demo_clipboard = result.clipboard_text;
            // Insertion changes wrapped rows; reveal the resulting caret using new metrics.
            const auto after = text_view::layout(state, options.viewport_size, 1.6f, options.multiline);
            gui::update_text_control_scroll(state, options, after.metrics);
        }

        [[nodiscard]] std::string edit_command(int command, std::string_view pasted = {})
        {
            auto* active = focused_text();
            if (!active) return {};
            constexpr std::array<gui::TextControlCommand, 6> commands{
                gui::TextControlCommand::select_all, gui::TextControlCommand::copy_selection,
                gui::TextControlCommand::cut_selection, gui::TextControlCommand::paste_text,
                gui::TextControlCommand::undo, gui::TextControlCommand::redo};
            if (command < 0 || static_cast<std::size_t>(command) >= commands.size()) return {};
            const std::string copied = command == 1 || command == 2 ? gui::selected_text(*active) : std::string{};
            if ((command == 1 || command == 2) && clipboard_write && !copied.empty()
                && !clipboard_write(clipboard_user, copied.c_str()))
            {
                edit_status = "CLIPBOARD UNAVAILABLE. YOUR TEXT HAS NOT BEEN CUT.";
                return {};
            }
            std::string paste_copy;
            if (command == 3 && pasted.empty())
            {
                if (clipboard_read)
                {
                    const char* source = clipboard_read(clipboard_user);
                    if (!source) { edit_status = "CLIPBOARD HAS NO READABLE TEXT."; return {}; }
                    paste_copy = source;
                }
                else paste_copy = demo_clipboard;
                pasted = paste_copy;
            }
            apply_text_command(*active, text_options(active_text_control), commands[command], pasted);
            edit_status = command == 3 ? "PASTED TEXT WRAPS AND SCROLLS INSIDE THE FIELD."
                : command == 1 || command == 2 ? "SELECTION COPIED. TRY PASTE IN ANOTHER FIELD."
                : "TEXT CONTROLLER UPDATED.";
            return copied;
        }

        void update_text_page(const input::InputFrame& frame)
        {
            const auto& left = frame.pointer(input::PointerButton::left);
            if (left.pressed)
            {
                bool hit{};
                for (std::uint32_t id = 1; id <= 3; ++id)
                {
                    const auto rect = text_rect(id);
                    if (!gui::contains(rect, frame.pointer_position)) continue;
                    active_text_control = id;
                    edit_state.focused = id == 1;
                    multiline_state.focused = id == 2;
                    static_state.focused = id == 3;
                    auto& state = *focused_text();
                    const auto options = text_options(id);
                    const auto view = text_view::layout(state, options.viewport_size, 1.6f, options.multiline);
                    const auto caret = text_view::caret_from_point(state, view,
                        {frame.pointer_position.x - rect.position.x, frame.pointer_position.y - rect.position.y});
                    apply_text_command(state, options, gui::TextControlCommand::set_caret, {}, caret, frame.modifiers.shift);
                    text_drag = true;
                    hit = true;
                    break;
                }
                if (!hit && !gui::contains({{270.0f, 520.0f}, {914.0f, 36.0f}}, frame.pointer_position))
                {
                    active_text_control = 0;
                    edit_state.focused = multiline_state.focused = static_state.focused = false;
                }
            }
            for (std::uint32_t id = 1; id <= 3; ++id)
            {
                auto& state = id == 1 ? edit_state : id == 2 ? multiline_state : static_state;
                const auto rect = text_rect(id);
                if (gui::contains(rect, frame.pointer_position) && frame.wheel_delta.y != 0.0f)
                {
                    const auto view = text_view::layout(state, rect.size, 1.6f, id != 1);
                    text_view::scroll_by(state, view, {0.0f, -frame.wheel_delta.y * view.line_height * 3.0f});
                }
            }
            auto* active = focused_text();
            if (!active) { pending_text.clear(); text_drag = false; return; }
            const auto options = text_options(active_text_control);
            if (text_drag && left.down && !left.pressed)
            {
                const auto rect = text_rect(active_text_control);
                const auto view = text_view::layout(*active, rect.size, 1.6f, options.multiline);
                const auto caret = text_view::caret_from_point(*active, view,
                    {frame.pointer_position.x - rect.position.x, frame.pointer_position.y - rect.position.y});
                apply_text_command(*active, options, gui::TextControlCommand::set_caret, {}, caret, true);
            }
            if (!left.down) text_drag = false;
            if (!pending_text.empty()) apply_text_command(*active, options, gui::TextControlCommand::insert_text, pending_text);
            pending_text.clear();
            const bool extend = frame.modifiers.shift;
            if (frame.key(input::Key::backspace).pressed) apply_text_command(*active, options, gui::TextControlCommand::erase_backward);
            if (frame.key(input::Key::delete_key).pressed) apply_text_command(*active, options, gui::TextControlCommand::erase_forward);
            if (frame.key(input::Key::arrow_left).pressed) apply_text_command(*active, options,
                frame.modifiers.control ? gui::TextControlCommand::move_word_left : gui::TextControlCommand::move_left, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::arrow_right).pressed) apply_text_command(*active, options,
                frame.modifiers.control ? gui::TextControlCommand::move_word_right : gui::TextControlCommand::move_right, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::arrow_up).pressed) apply_text_command(*active, options, gui::TextControlCommand::move_up, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::arrow_down).pressed) apply_text_command(*active, options, gui::TextControlCommand::move_down, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::home).pressed) apply_text_command(*active, options,
                frame.modifiers.control ? gui::TextControlCommand::move_document_start : gui::TextControlCommand::move_line_start, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::end).pressed) apply_text_command(*active, options,
                frame.modifiers.control ? gui::TextControlCommand::move_document_end : gui::TextControlCommand::move_line_end, {}, gui::invalid_text_index, extend);
            if (frame.key(input::Key::enter).pressed && active_text_control == 2)
                apply_text_command(*active, options, gui::TextControlCommand::insert_text, "\n");
            if (frame.key(input::Key::tab).pressed && active_text_control == 2 && !frame.modifiers.control)
                apply_text_command(*active, options, gui::TextControlCommand::insert_text, "\t");
            if (left.released)
                for (int command = 0; command < 6; ++command)
                    if (gui::contains({{270.0f + command * 152.0f, 520.0f}, {144.0f, 32.0f}}, frame.pointer_position))
                        (void)edit_command(command);
        }

        void update_image_page(const input::InputFrame& frame) noexcept
        {
            const gui::Rect toggle{ { 270.0f, 188.0f }, { 230.0f, 34.0f } };
            if (frame.pointer(input::PointerButton::left).released
                && gui::contains(toggle, frame.pointer_position))
            {
                image_fit = !image_fit;
            }
        }

        void apply_context_action(std::uint32_t item) noexcept
        {
            if (context_text)
            {
                (void)edit_command(static_cast<int>(item));
                context_popup.open = false;
                context_layout.visible = false;
                return;
            }
            switch (item)
            {
            case 0:
                page = 1;
                core_window.open = true;
                core_window.position = { context_anchor.x, (std::max)(workspace_top + 8.0f, context_anchor.y) };
                core_window.focus_order = next_focus_order++;
                break;
            case 1:
                page = 1;
                rounded_window.open = true;
                rounded_window.position = { context_anchor.x, (std::max)(workspace_top + 8.0f, context_anchor.y) };
                rounded_window.focus_order = next_focus_order++;
                break;
            case 2:
                reset_windows();
                reset_docking();
                break;
            case 3:
                native_frame_enabled = !native_frame_enabled;
                pending_native_frame_mode = native_frame_enabled ? 1 : 0;
                break;
            default:
                break;
            }
            context_popup.open = false;
            context_layout.visible = false;
        }

        void update_context_menu(const input::InputFrame& frame) noexcept
        {
            const input::ContextMenuRequest request = input::context_menu_request(frame, content_area());
            if (request.requested)
            {
                context_anchor = request.position;
                context_text = false;
                if (page == 0 && showcase_open[1])
                {
                    const auto local = section_input(frame, 1);
                    for (std::uint32_t id = 1; id <= 3; ++id)
                        if (gui::contains(text_rect(id), local.pointer_position))
                        {
                            context_text = true;
                            active_text_control = id;
                            edit_state.focused = id == 1;
                            multiline_state.focused = id == 2;
                            static_state.focused = id == 3;
                        }
                }
            }
            const std::uint32_t count = context_text ? 6u : EPOCHGUI_DEMO_PLATFORM_HOSTED ? 3u : 4u;
            const gui::PopupOptions options{
                .owner = { context_anchor, { 1.0f, 1.0f } },
                .preferred_size = { 248.0f, static_cast<float>(count) * 36.0f + 20.0f },
                .viewport_size = { canvas_width, canvas_height },
                .cursor_offset = { 0.0f, 0.0f },
                .placement = gui::PopupPlacement::below,
                .gap = 0.0f,
                .margin = 8.0f,
                .flip_to_fit = true,
                .clamp_to_viewport = true,
                .close_on_outside_press = true
            };
            context_layout = gui::update_popup(context_popup, options, input::popup_input(frame, request.requested));
            if (context_layout.visible && frame.pointer(input::PointerButton::left).released)
            {
                for (std::uint32_t item = 0; item < count; ++item)
                {
                    const gui::Rect item_rect{
                        { context_layout.popup.position.x + 10.0f, context_layout.popup.position.y + 10.0f + static_cast<float>(item) * 36.0f },
                        { context_layout.popup.size.x - 20.0f, 30.0f }
                    };
                    if (gui::contains(item_rect, frame.pointer_position))
                    {
                        apply_context_action(item);
                        break;
                    }
                }
            }
        }

        void update_ui()
        {
            const input::InputFrame& frame = input_tracker.frame();
            if (pending_workspace_step != 0)
            {
                constexpr std::array<std::uint32_t, 3> pages{0, 1, 5};
                const auto current = std::find(pages.begin(), pages.end(), page);
                const std::size_t index = current == pages.end() ? 0u : static_cast<std::size_t>(current - pages.begin());
                page = pages[(index + (pending_workspace_step < 0 ? 2u : 1u)) % pages.size()];
                pending_workspace_step = 0;
                pending_text.clear();
                active_text_control = 0;
                edit_state.focused = multiline_state.focused = static_state.focused = false;
                return;
            }
            const input::WindowCommand command = input::borderless_window_command(chrome_layout, frame);
            if (command != input::WindowCommand::none)
                pending_window_command = command;
            const bool menu_was_open = context_popup.open;
            update_context_menu(frame);
            if (context_popup.open || (menu_was_open && frame.pointer(input::PointerButton::left).released))
                return;
            update_navigation(frame);
            update_frame_toggle(frame);

            switch (page)
            {
            case 0: update_showcase(frame); break;
            case 1: update_windows_page(frame); break;
            case 2: update_text_page(frame); break;
            case 3: update_image_page(frame); break;
            case 4: break;
            case 5: break; // Libraries updates its controls while building the page.
            default: page = 0; break;
            }

            if (frame.key(input::Key::escape).pressed && context_popup.open)
                context_popup.open = false;
            if (page == 0 && frame.key(input::Key::tab).pressed && active_text_control != 2)
            {
                active_text_control = active_text_control == 0 ? 1
                    : frame.modifiers.shift ? (active_text_control == 1 ? 3 : active_text_control - 1)
                    : (active_text_control == 3 ? 1 : active_text_control + 1);
                showcase_open[1] = true;
                showcase_scroll += section_header(1).position.y - showcase_viewport().position.y;
                edit_state.focused = active_text_control == 1;
                multiline_state.focused = active_text_control == 2;
                static_state.focused = active_text_control == 3;
            }
        }

        void draw_custom_title_bar(const input::InputFrame& frame)
        {
            batch.append_rect(chrome_layout.title_bar, title_fill);
            batch.append_rect({ { 0.0f, custom_title_height - 1.0f }, { canvas_width, 1.0f } }, panel_border);
            draw_text(batch, 20.0f, 18.0f, "EPOCHGUI SHOWCASE", 2.15f, text_primary);
            draw_text(batch, 360.0f, 21.0f, "LIVE WIDGETS - WINDOWS - LIBRARIES", 1.35f, text_secondary);

            const input::WindowChromeRegion hovered = input::hit_test_borderless_window_chrome(chrome_layout, frame.pointer_position);
            const auto draw_button = [&](gui::Rect rect, input::WindowChromeRegion region, Color active, std::string_view label)
            {
                const bool is_hovered = hovered == region;
                batch.append_rect(rect, is_hovered ? active : title_fill);
                draw_text(batch, rect.position.x + 17.0f, rect.position.y + 20.0f, label, 1.35f,
                    is_hovered ? text_primary : text_secondary);
            };
            draw_button(chrome_layout.minimize_button, input::WindowChromeRegion::minimize_button, control_hover, "-");
            draw_button(chrome_layout.maximize_button, input::WindowChromeRegion::maximize_button, control_hover, "O");
            draw_button(chrome_layout.close_button, input::WindowChromeRegion::close_button, danger, "X");
        }

        void draw_toolbar(const input::InputFrame& frame)
        {
            batch.append_rect({ { 0.0f, custom_title_height }, { canvas_width, toolbar_height } }, toolbar_fill);
            batch.append_rect({ { 0.0f, workspace_top - 1.0f }, { canvas_width, 1.0f } }, panel_border);
            constexpr std::array<float, 3> widths{ 150.0f,174.0f,160.0f };
            constexpr std::array<std::string_view, 3> labels{ "WIDGETS","WINDOWS / DOCKS","LIBRARIES" };
            constexpr std::array<std::uint32_t, 3> pages{0, 1, 5};
            const gui::SegmentedControlLayoutOptions options{
                .position = { 22.0f, 67.0f },
                .item_widths = widths,
                .height = 32.0f,
                .gap = 4.0f
            };
            for (std::uint32_t index = 0; index < labels.size(); ++index)
            {
                const gui::Rect item = gui::segmented_control_item_layout(options, index);
                const bool hovered = gui::contains(item, frame.pointer_position);
                const bool selected = page == pages[index];
                draw_surface(batch, item, true, 7.0f,
                    selected ? control_selected : hovered ? control_hover : control_fill,
                    selected ? accent : panel_border,
                    1.0f);
                draw_text(batch, item.position.x + 12.0f, item.position.y + 11.0f, labels[index], 1.25f,
                    selected ? text_primary : text_secondary);
            }

            if (EPOCHGUI_DEMO_PLATFORM_HOSTED) return;
            const gui::Rect toggle = frame_toggle_rect();
            const bool hovered = gui::contains(toggle, frame.pointer_position);
            draw_surface(batch, toggle, true, 16.0f,
                native_frame_enabled ? accent_soft : warning,
                hovered ? accent : panel_border,
                1.0f);
            draw_text(batch, toggle.position.x + 18.0f, toggle.position.y + 11.0f,
                native_frame_enabled ? "NATIVE FRAME ON" : "BORDERLESS MODE",
                1.35f,
                native_frame_enabled ? text_primary : background);
        }

        void draw_sidebar()
        {
            batch.append_rect({{0.0f, workspace_top}, {sidebar_width, canvas_height - workspace_top}}, sidebar_fill);
            batch.append_rect({{sidebar_width - 1.0f, workspace_top}, {1.0f, canvas_height - workspace_top}}, panel_border);
            draw_text(batch, 18.0f, 140.0f, "EXPLORE", 1.6f, text_primary);
            constexpr std::array<std::string_view, 4> sections{"CONTROLS", "TEXT EDITING", "IMAGES", "INPUT"};
            for (std::size_t index = 0; index < sections.size(); ++index)
            {
                const gui::Rect button{{14.0f, 180.0f + static_cast<float>(index) * 52.0f}, {186.0f, 42.0f}};
                draw_surface(batch, button, true, 6.0f, page == 0 && showcase_open[index] ? control_selected : control_fill,
                    panel_border, 1.0f);
                draw_text(batch, button.position.x + 12.0f, button.position.y + 15.0f, sections[index], 1.2f, text_primary);
            }
            draw_text(batch, 18.0f, 430.0f, "ONE SHOWCASE", 1.3f, accent);
            draw_text(batch, 18.0f, 458.0f, "SCROLL TO EXPLORE.", 1.1f, text_secondary);
            draw_text(batch, 18.0f, 482.0f, "CLICK A HEADING", 1.1f, text_secondary);
            draw_text(batch, 18.0f, 506.0f, "TO FOLD A SECTION.", 1.1f, text_secondary);
            draw_text(batch, 18.0f, 566.0f, "WINDOWS / DOCKS", 1.15f, warning);
            draw_text(batch, 18.0f, 592.0f, "DRAG, RESIZE AND", 1.1f, text_secondary);
            draw_text(batch, 18.0f, 616.0f, "REARRANGE PANELS.", 1.1f, text_secondary);
            draw_text(batch, 18.0f, 706.0f, "EPOCHGUI " + std::string(gui::version_string), 1.15f, accent);
            draw_text(batch, 18.0f, 744.0f, "CTRL + TAB", 1.05f, text_muted);
            draw_text(batch, 18.0f, 766.0f, "CHANGES WORKSPACE", 1.05f, text_muted);
        }

        void draw_overview(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "CORE LAYOUT PRIMITIVES", 1.8f, text_primary);
            draw_text(batch, 246.0f, 168.0f, "RECT HIT TEST - SEGMENTED - PROGRESS - LISTS - LOADING - SPLITTER", 1.15f, text_muted);

            const gui::Rect hit_rect{ { 270.0f, 210.0f }, { 280.0f, 44.0f } };
            const bool hit = gui::contains(hit_rect, frame.pointer_position);
            draw_surface(batch, hit_rect, false, 0.0f, hit ? control_hover : control_fill, hit ? accent : panel_border, 1.0f);
            draw_text(batch, 290.0f, 225.0f, hit ? "RECT CONTAINS POINTER" : "MOVE POINTER HERE", 1.3f, hit ? text_primary : text_secondary);

            const gui::SelectableListLayoutOptions list_options{
                .viewport = { { 270.0f, 278.0f }, { 280.0f, 190.0f } },
                .row_count = 5,
                .row_height = 30.0f,
                .row_gap = 4.0f,
                .content_padding_x = 8.0f,
                .content_padding_y = 8.0f
            };
            draw_surface(batch, list_options.viewport, true, 10.0f, panel_fill, panel_border, 1.0f);
            constexpr std::array<std::string_view, 5> rows{ "SCENE", "INSPECTOR", "ASSETS", "CONSOLE", "PROFILER" };
            for (std::uint32_t index = 0; index < rows.size(); ++index)
            {
                const gui::SelectableRowLayout row = gui::make_selectable_row_layout(list_options, index, frame.pointer_position, selected_row == index);
                if (!row.visible)
                    continue;
                draw_surface(batch, row.row, true, 6.0f,
                    row.selected ? control_selected : row.hovered ? control_hover : control_fill,
                    row.selected ? accent : panel_border,
                    row.selected ? 1.0f : 0.0f);
                draw_text(batch, row.content.position.x + 8.0f, row.content.position.y + 10.0f, rows[index], 1.35f,
                    row.selected ? text_primary : text_secondary);
            }

            constexpr std::array<gui::ProgressBarDirection, 4> directions{
                gui::ProgressBarDirection::left_to_right,
                gui::ProgressBarDirection::right_to_left,
                gui::ProgressBarDirection::top_to_bottom,
                gui::ProgressBarDirection::bottom_to_top
            };
            for (std::size_t index = 0; index < directions.size(); ++index)
            {
                const bool vertical = index >= 2;
                const gui::Rect track{
                    { 620.0f + static_cast<float>(index) * 135.0f, 220.0f },
                    vertical ? gui::Vec2{ 36.0f, 180.0f } : gui::Vec2{ 118.0f, 30.0f }
                };
                const gui::ProgressBarLayout progress = gui::make_progress_bar_layout({
                    .track = track,
                    .value = demo_value,
                    .minimum = 0.0f,
                    .maximum = 1.0f,
                    .padding = 3.0f,
                    .direction = directions[index]
                });
                draw_surface(batch, progress.track, true, 8.0f, control_fill, panel_border, 1.0f);
                draw_surface(batch, progress.fill, true, 6.0f, index % 2 == 0 ? accent : green, panel_border, 0.0f);
            }

            const auto value_slider = gui::make_slider_layout({
                .bounds = {{620.0f, 398.0f}, {270.0f, 28.0f}}, .value = demo_value,
                .thumb_width = 14.0f, .thumb_height = 20.0f});
            batch.append_rect(value_slider.track, control_fill);
            batch.append_rect(value_slider.fill, accent);
            draw_surface(batch, value_slider.thumb, true, 5.0f, text_primary, accent, 1.0f);
            const auto help_toggle = gui::make_toggle_switch_layout({
                .bounds = {{940.0f, 398.0f}, {52.0f, 26.0f}}, .value = show_tips});
            draw_surface(batch, help_toggle.track, true, 12.0f, show_tips ? accent : control_fill, panel_border, 1.0f);
            draw_surface(batch, help_toggle.thumb, true, 10.0f, text_primary, text_primary, 0.0f);
            draw_text(batch, 1008.0f, 406.0f, "SHOW TIPS", 1.1f, text_secondary);

            const gui::LoadingScreenLayout loading = gui::make_loading_screen_layout({
                .viewport = { { 620.0f, 430.0f }, { 570.0f, 150.0f } },
                .preferred_panel_size = { 570.0f, 150.0f },
                .minimum_panel_size = { 400.0f, 120.0f },
                .margin = 0.0f,
                .padding = 14.0f,
                .gap = 6.0f,
                .title_height = 16.0f,
                .message_height = 20.0f,
                .progress_height = 20.0f,
                .status_height = 14.0f,
                .action_height = 24.0f,
                .progress_padding = 2.0f,
                .progress_value = demo_value
            });
            draw_surface(batch, loading.panel, true, 12.0f, panel_fill, panel_border, 1.0f);
            draw_text(batch, loading.title.position.x, loading.title.position.y + 2.0f, "LOADING SCREEN LAYOUT", 1.35f, text_primary);
            draw_surface(batch, loading.progress.track, true, 6.0f, background, panel_border, 1.0f);
            draw_surface(batch, loading.progress.fill, true, 5.0f, green, green, 0.0f);
            draw_text(batch, loading.status.position.x, loading.status.position.y + 1.0f, std::to_string(static_cast<int>(demo_value * 100.0f)) + " PERCENT", 1.15f, text_secondary);

            const gui::SplitterLayoutOptions split_options{
                .area = { { 250.0f, 610.0f }, { 940.0f, 142.0f } },
                .axis = gui::SplitterAxis::vertical,
                .split_fraction = splitter_fraction,
                .thickness = 8.0f,
                .min_before = 180.0f,
                .min_after = 260.0f
            };
            const gui::SplitterLayout split = gui::make_splitter_layout(split_options);
            draw_surface(batch, split.before, true, 9.0f, panel_fill, panel_border, 1.0f);
            draw_surface(batch, split.after, true, 9.0f, control_fill, panel_border, 1.0f);
            draw_surface(batch, split.handle, true, 4.0f, accent, accent, 0.0f);
            if (show_tips)
            {
                draw_text(batch, split.before.position.x + 20.0f, split.before.position.y + 20.0f, "DRAG SPLITTER", 1.4f, text_secondary);
                draw_text(batch, split.after.position.x + 20.0f, split.after.position.y + 20.0f, "AFTER PANE", 1.4f, text_secondary);
            }
        }

        void draw_floating_window(
            const gui::FloatingWindowState& state,
            const gui::FloatingWindowLayout& layout,
            bool rounded_surface,
            bool active)
        {
            if (!state.open || !layout.visible)
                return;
            draw_surface(batch, layout.window, rounded_surface, 12.0f, panel_fill, active ? accent : panel_border, active ? 2.0f : 1.0f);
            draw_surface(batch, layout.title_bar, rounded_surface, 10.0f, active ? control_selected : control_fill, panel_border, 0.0f);
            batch.append_rect({ { layout.title_bar.position.x, layout.title_bar.position.y + layout.title_bar.size.y - 9.0f }, { layout.title_bar.size.x, 9.0f } }, active ? control_selected : control_fill);
            draw_text(batch, layout.title_bar.position.x + 12.0f, layout.title_bar.position.y + 10.0f,
                rounded_surface ? "ROUNDED FLOATING WINDOW" : "CORE FLOATING WINDOW",
                1.25f,
                text_primary, (std::max)(0.0f, layout.title_bar.size.x - 62.0f));
            draw_surface(batch, layout.close_button, true, 6.0f, layout.close_hovered ? danger : warning, danger, 0.0f);
            draw_text(batch, layout.close_button.position.x + 8.0f, layout.close_button.position.y + 7.0f, "X", 1.1f, background);
            ClipScope floating_content_clip{batch, layout.content};
            draw_text(batch, layout.content.position.x + 10.0f, layout.content.position.y + 12.0f,
                "DRAG TITLE - RESIZE CORNER - CLOSE WORKS",
                1.1f,
                text_secondary,
                layout.content.size.x - 20.0f);
            const gui::ProgressBarLayout progress = gui::make_progress_bar_layout({
                .track = { { layout.content.position.x + 10.0f, layout.content.position.y + 46.0f }, { layout.content.size.x - 20.0f, 20.0f } },
                .value = rounded_surface ? 0.78f : 0.52f,
                .minimum = 0.0f,
                .maximum = 1.0f,
                .padding = 3.0f
            });
            draw_surface(batch, progress.track, rounded_surface, 6.0f, background, panel_border, 1.0f);
            draw_surface(batch, progress.fill, rounded_surface, 5.0f, rounded_surface ? green : accent, panel_border, 0.0f);
            draw_surface(batch, layout.resize_handle, true, 4.0f, layout.resize_hovered ? accent : panel_border, panel_border, 0.0f);
        }

        void draw_windows_page(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "WINDOWING AND HOSTING", 1.8f, text_primary);
            constexpr std::array<std::string_view, 3> labels{"RECT", "ROUNDED", "BOTH"};
            const auto mode_options = window_mode_options();
            for (std::uint32_t index = 0; index < labels.size(); ++index)
            {
                const gui::Rect item = gui::segmented_control_item_layout(mode_options, index);
                draw_surface(batch, item, true, 7.0f,
                    display_mode == index ? control_selected : gui::contains(item, frame.pointer_position) ? control_hover : control_fill,
                    display_mode == index ? accent : panel_border,
                    1.0f);
                draw_text(batch, item.position.x + 16.0f, item.position.y + 11.0f, labels[index], 1.25f,
                    display_mode == index ? text_primary : text_secondary);
            }

            const auto dock_options = window_dock_options();
            const gui::DockLayoutInput dock_input{ .mouse_position = frame.pointer_position };
            constexpr std::array<std::string_view, 3> pane_names{ "TREE", "VIEWPORT", "INSPECTOR" };
            draw_text(batch, 246.0f, 400.0f, "CLICK DOCK PANE TITLE TO POPOUT OR REDOCK", 1.15f, text_muted);
            for (const bool popped_out : {false, true})
            for (std::size_t index = 0; index < dock_panes.size(); ++index)
            {
                if (dock_panes[index].popped_out != popped_out) continue;
                const gui::DockPaneLayout pane = gui::make_dock_pane_layout(dock_panes[index], dock_options, dock_input);
                if (!pane.visible)
                    continue;
                draw_surface(batch, pane.frame, true, 8.0f, pane.active ? control_selected : panel_fill, pane.active ? accent : panel_border, 1.0f);
                draw_surface(batch, pane.title_bar, true, 7.0f, control_fill, panel_border, 0.0f);
                draw_text(batch, pane.title_bar.position.x + 8.0f, pane.title_bar.position.y + 9.0f, pane_names[index], 1.15f, text_secondary);
            }

            const bool core_visible = core_window.open && display_mode != 1;
            const bool rounded_visible = rounded_window.open && display_mode != 0;
            const bool core_active = core_visible && (!rounded_visible || core_window.focus_order >= rounded_window.focus_order);
            if (core_active)
            {
                draw_floating_window(rounded_window, rounded_layout, true, false);
                draw_floating_window(core_window, core_layout, false, true);
            }
            else
            {
                draw_floating_window(core_window, core_layout, false, false);
                draw_floating_window(rounded_window, rounded_layout, true, rounded_visible);
            }

            // Present the same Gui-owned frames that receive actions and hit tests.
            const auto& hosted = dockable_result.chrome;
            if (hosted.visible)
            {
                draw_surface(batch, hosted.frame, true, 7.0f, panel_fill, accent, 1.0f);
                draw_surface(batch, hosted.title_bar, true, 6.0f, control_fill, panel_border, 0.0f);
                draw_text(batch, hosted.title_bar.position.x + 6.0f, hosted.title_bar.position.y + 9.0f,
                    "HOST", 1.0f, text_primary);
                const std::array<gui::Rect, 4> actions{hosted.dock_button, hosted.float_button,
                    hosted.detach_button, hosted.close_button};
                constexpr std::array<std::string_view, 4> names{"DOCK", "FLOAT", "OUT", "X"};
                for (std::size_t index = 0; index < actions.size(); ++index)
                {
                    if (actions[index].size.x <= 0 || actions[index].size.y <= 0) continue;
                    draw_surface(batch, actions[index], true, 3.0f, control_hover, panel_border, 1.0f);
                    draw_text(batch, actions[index].position.x + 3.0f, actions[index].position.y + 8.0f,
                        names[index], 0.9f, text_primary);
                }
                ClipScope content_clip(batch, hosted.content);
                const char* mode = dockable_result.mode == gui::DockableWindowMode::docked ? "DOCKED"
                    : dockable_result.mode == gui::DockableWindowMode::floating ? "FLOATING"
                    : "DETACHED LAYOUT";
                draw_text(batch, hosted.content.position.x + 8.0f, hosted.content.position.y + 12.0f,
                    mode, 1.2f, text_primary);
                draw_text(batch, hosted.content.position.x + 8.0f, hosted.content.position.y + 36.0f,
                    "GUI GEOMETRY ONLY", 1.0f, text_secondary);
            }
            const auto& panel = panel_host_result.metadata;
            if (panel.visible)
            {
                draw_surface(batch, panel.frame, true, 7.0f, panel_fill, green, 1.0f);
                ClipScope content_clip(batch, panel.frame);
                draw_text(batch, panel.frame.position.x + 10.0f, panel.frame.position.y + 12.0f,
                    "PANEL HOST", 1.2f, text_primary);
                draw_text(batch, panel.frame.position.x + 10.0f, panel.frame.position.y + 38.0f,
                    panel.mode == gui::PanelHostMode::floating ? "FLOATING" : "DOCKED", 1.1f, green);
            }

            constexpr std::array<gui::Rect, 3> dock_buttons{
                gui::Rect{ { 246.0f, 704.0f }, { 142.0f, 30.0f } },
                gui::Rect{ { 396.0f, 704.0f }, { 142.0f, 30.0f } },
                gui::Rect{ { 546.0f, 704.0f }, { 142.0f, 30.0f } }
            };
            constexpr std::array<std::string_view, 3> dock_labels{ "DOCK", "FLOAT", "DETACH" };
            for (std::size_t index = 0; index < dock_buttons.size(); ++index)
            {
                draw_surface(batch, dock_buttons[index], true, 7.0f,
                    gui::contains(dock_buttons[index], frame.pointer_position) ? control_hover : control_fill,
                    panel_border,
                    1.0f);
                draw_text(batch, dock_buttons[index].position.x + 18.0f, dock_buttons[index].position.y + 10.0f, dock_labels[index], 1.2f, text_secondary);
            }
            const gui::Rect panel_button{ { 970.0f, 704.0f }, { 220.0f, 30.0f } };
            draw_surface(batch, panel_button, true, 7.0f, control_fill, panel_border, 1.0f);
            draw_text(batch, panel_button.position.x + 16.0f, panel_button.position.y + 10.0f, "TOGGLE PANEL HOST", 1.15f, text_secondary);
        }

        void draw_text_control(
            gui::Rect rect,
            const gui::TextControlState& state,
            bool read_only,
            std::string_view label)
        {
            draw_text(batch, rect.position.x, rect.position.y - 26.0f, label, 1.3f, text_secondary);
            draw_surface(batch, rect, true, 9.0f,
                state.focused ? control_hover : panel_fill,
                state.focused ? accent : panel_border,
                state.focused ? 2.0f : 1.0f);
            const auto view = text_view::layout(state, rect.size, 1.6f, rect.size.y > 60.0f);
            ClipScope text_clip{batch, inset(rect, 5.0f)};
            if (state.focused)
                for (auto selected : text_view::selection_rectangles(state, view))
                {
                    selected.position.x += rect.position.x;
                    selected.position.y += rect.position.y;
                    batch.append_rect(selected, control_selected);
                }
            for (std::size_t row = 0; row < view.lines.size(); ++row)
            {
                const float y = rect.position.y + view.text_origin.y + static_cast<float>(row) * view.line_height;
                if (y + view.line_height < rect.position.y || y > rect.position.y + rect.size.y) continue;
                float x = rect.position.x + view.text_origin.x;
                const auto& line = view.lines[row];
                for (std::size_t index = line.first; index < line.past_last;)
                {
                    const auto glyph = text_view::glyph(state.text, index, view.scale);
                    const auto byte = static_cast<unsigned char>(state.text[index]);
                    if (byte != '\t' && byte != '\n' && byte != '\r')
                    {
                        const char character = byte < 128 ? static_cast<char>(byte) : '?';
                        draw_text(batch, x, y, std::string_view{&character, 1}, view.scale,
                            read_only ? text_secondary : text_primary);
                    }
                    x += glyph.advance;
                    index += glyph.byte_count;
                }
            }
            if (state.focused && !read_only)
            {
                batch.append_rect({
                    {rect.position.x + view.text_origin.x + view.metrics.caret_position.x,
                     rect.position.y + view.text_origin.y + view.metrics.caret_position.y},
                    {2.0f, view.metrics.caret_size.y}
                }, accent);
            }
            if (view.maximum_scroll.y > 0.0f)
            {
                const auto scroll = gui::make_scrollbar_layout({
                    .track = {{rect.position.x + rect.size.x - 8.0f, rect.position.y + 6.0f}, {3.0f, rect.size.y - 12.0f}},
                    .content_extent = view.metrics.content_size.y + 8.0f,
                    .scroll_offset = state.scroll.y, .minimum_thumb_extent = 14.0f});
                batch.append_rect(scroll.track, background);
                batch.append_rect(scroll.thumb, accent);
            }
        }

        void draw_text_page()
        {
            draw_text(batch, 246.0f, 140.0f, "TEXT CONTROLS", 1.8f, text_primary);
            draw_text(batch, 246.0f, 168.0f, "EDIT BOX - MULTILINE EDITOR - READ ONLY STATIC TEXT", 1.2f, text_muted);
            draw_text_control(text_rect(1), edit_state, false, "PROJECT NAME");
            draw_text_control(text_rect(2), multiline_state, false, "NOTES - WRAPS AND SCROLLS");
            draw_text_control(text_rect(3), static_state, true, "READ ONLY - SELECT AND COPY");
            constexpr std::array<std::string_view, 6> edit_labels{"SELECT ALL", "COPY", "CUT", "PASTE", "UNDO", "REDO"};
            for (std::size_t command = 0; command < edit_labels.size(); ++command)
            {
                const gui::Rect button{{270.0f + static_cast<float>(command) * 152.0f, 520.0f}, {144.0f, 32.0f}};
                draw_surface(batch, button, true, 6.0f, control_fill, panel_border, 1.0f);
                draw_text(batch, button.position.x + 12.0f, button.position.y + 11.0f,
                    edit_labels[command], 1.2f, active_text_control ? text_primary : text_muted);
            }

            draw_surface(batch, { { 270.0f, 554.0f }, { 914.0f, 150.0f } }, true, 12.0f, panel_fill, panel_border, 1.0f);
            draw_text(batch, 294.0f, 580.0f, "TRY IT", 1.45f, text_primary);
            draw_text(batch, 294.0f, 614.0f, "DRAG TO SELECT. SHIFT + ARROWS EXTENDS THE SELECTION.", 1.2f, text_secondary);
            draw_text(batch, 294.0f, 640.0f, "WINDOWS: CTRL + A / C / X / V / Z / Y USES YOUR CLIPBOARD.", 1.2f, text_secondary);
            draw_text(batch, 294.0f, 670.0f, edit_status, 1.2f, green, 870.0f);
        }

        void draw_image_page(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "IMAGE LOADING AND DISPLAY", 1.8f, text_primary);
            draw_text(batch, 246.0f, 168.0f, "PPM P3 OR P6 DECODED BY EPOCHGUI - DEMO BATCHES OPENGL PIXELS", 1.15f, text_muted);
            const gui::Rect toggle{ { 270.0f, 188.0f }, { 230.0f, 34.0f } };
            draw_surface(batch, toggle, true, 8.0f,
                gui::contains(toggle, frame.pointer_position) ? control_hover : control_fill,
                accent,
                1.0f);
            draw_text(batch, toggle.position.x + 18.0f, toggle.position.y + 12.0f,
                image_fit ? "MODE FIT" : "MODE STRETCH",
                1.3f,
                text_primary);
            draw_image(batch, demo_image, { { 270.0f, 242.0f }, { 620.0f, 430.0f } }, image_fit);
            draw_surface(batch, { { 920.0f, 242.0f }, { 270.0f, 430.0f } }, true, 12.0f, panel_fill, panel_border, 1.0f);
            draw_text(batch, 944.0f, 272.0f, "IMAGE STATUS", 1.45f, text_primary);
            draw_text(batch, 944.0f, 314.0f, demo_image.valid() ? "LOADED" : "NOT FOUND", 1.45f,
                demo_image.valid() ? green : danger);
            draw_text(batch, 944.0f, 354.0f, ppm_format_label(demo_image), 1.2f, text_secondary);
            draw_text(batch, 944.0f, 382.0f, "SIZE 16 X 10", 1.2f, text_secondary);
            draw_text(batch, 944.0f, 430.0f, "DECODER AND FIT", 1.2f, warning);
            draw_text(batch, 944.0f, 456.0f, "LIVE IN", 1.2f, text_muted);
            draw_text(batch, 944.0f, 482.0f, "EPOCHGUI IMAGE", 1.2f, text_muted);
            draw_text(batch, 944.0f, 536.0f, "OPENGL BATCHING", 1.2f, text_secondary);
            draw_text(batch, 944.0f, 562.0f, "STAYS IN", 1.2f, text_secondary);
            draw_text(batch, 944.0f, 588.0f, "THE DEMO", 1.2f, text_secondary);
        }

        void draw_input_page(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "OPTIONAL INPUT FALLBACK AND CUSTOM CHROME", 1.8f, text_primary);
            draw_text(batch, 246.0f, 168.0f, "DISABLED BY DEFAULT IN EPOCHGUI - ENABLED ONLY FOR THIS DEMO", 1.15f, text_muted);

            draw_surface(batch, { { 270.0f, 210.0f }, { 430.0f, 360.0f } }, true, 12.0f, panel_fill, panel_border, 1.0f);
            draw_text(batch, 294.0f, 238.0f, "LIVE INPUT FRAME", 1.45f, text_primary);
            draw_text(batch, 294.0f, 282.0f,
                frame.pointer(input::PointerButton::left).down ? "LEFT DOWN" : "LEFT UP",
                1.35f,
                frame.pointer(input::PointerButton::left).down ? green : text_secondary);
            draw_text(batch, 294.0f, 310.0f,
                frame.pointer(input::PointerButton::right).down ? "RIGHT DOWN" : "RIGHT UP",
                1.35f,
                frame.pointer(input::PointerButton::right).down ? warning : text_secondary);
            draw_text(batch, 294.0f, 338.0f,
                frame.pointer(input::PointerButton::middle).down ? "MIDDLE DOWN" : "MIDDLE UP",
                1.35f,
                frame.pointer(input::PointerButton::middle).down ? accent : text_secondary);
            draw_text(batch, 294.0f, 390.0f, frame.modifiers.shift ? "SHIFT ON" : "SHIFT OFF", 1.25f, text_muted);
            draw_text(batch, 294.0f, 416.0f, frame.modifiers.control ? "CONTROL ON" : "CONTROL OFF", 1.25f, text_muted);
            draw_text(batch, 294.0f, 442.0f, frame.modifiers.alt ? "ALT ON" : "ALT OFF", 1.25f, text_muted);
            draw_text(batch, 294.0f, 492.0f, "RIGHT CLICK OPENS POPUP", 1.2f, green);
            draw_text(batch, 294.0f, 520.0f, "CTRL + TAB CHANGES WORKSPACE", 1.2f, text_secondary);

            draw_surface(batch, { { 742.0f, 210.0f }, { 448.0f, 360.0f } }, true, 12.0f, panel_fill, panel_border, 1.0f);
            draw_text(batch, 766.0f, 238.0f, "CUSTOM TITLE BAR REGIONS", 1.45f, text_primary);
            draw_text(batch, 766.0f, 282.0f, "CAPTION", 1.2f, text_secondary);
            draw_text(batch, 766.0f, 310.0f, "CLIENT", 1.2f, text_secondary);
            draw_text(batch, 766.0f, 338.0f, "RESIZE EDGES AND CORNERS", 1.2f, text_secondary);
            draw_text(batch, 766.0f, 366.0f, "MINIMIZE MAXIMIZE CLOSE", 1.2f, text_secondary);
            draw_text(batch, 766.0f, 414.0f, "CUSTOM BAR ALWAYS DRAWN", 1.2f, green);
            draw_text(batch, 766.0f, 442.0f, "NATIVE OUTER FRAME TOGGLES", 1.2f, warning);
            draw_text(batch, 766.0f, 470.0f, native_frame_enabled ? "CURRENT NATIVE FRAME ON" : "CURRENT BORDERLESS MODE", 1.2f,
                native_frame_enabled ? accent : warning);

            draw_surface(batch, { { 270.0f, 610.0f }, { 920.0f, 110.0f } }, true, 12.0f, control_fill, panel_border, 1.0f);
            draw_text(batch, 294.0f, 640.0f, "EXISTING ENGINE SDL GLFW OR PLATFORM INPUT SHOULD REMAIN PRIMARY", 1.25f, text_primary);
            draw_text(batch, 294.0f, 674.0f, "EPOCH.GUI.INPUT IS ONLY THE PORTABLE FALLBACK PATH", 1.25f, text_secondary);
        }

        void draw_context_menu(const input::InputFrame& frame)
        {
            if (!context_layout.visible)
                return;
            draw_surface(batch, context_layout.popup, true, 10.0f, panel_fill, accent, 1.5f);
            constexpr std::array<std::string_view, 4> labels{
                "OPEN CORE WINDOW", "OPEN ROUNDED WINDOW", "RESET WINDOWS", "TOGGLE NATIVE FRAME"
            };
            constexpr std::array<std::string_view, 6> edit_labels{"SELECT ALL", "COPY", "CUT", "PASTE", "UNDO", "REDO"};
            const std::size_t count = context_text ? edit_labels.size() : EPOCHGUI_DEMO_PLATFORM_HOSTED ? 3u : labels.size();
            ClipScope menu_clip{batch, context_layout.popup};
            for (std::uint32_t index = 0; index < count; ++index)
            {
                const gui::Rect item{
                    { context_layout.popup.position.x + 10.0f, context_layout.popup.position.y + 10.0f + static_cast<float>(index) * 36.0f },
                    { context_layout.popup.size.x - 20.0f, 30.0f }
                };
                const bool hovered = gui::contains(item, frame.pointer_position);
                draw_surface(batch, item, true, 6.0f,
                    hovered ? control_hover : background,
                    hovered ? accent_soft : background,
                    0.0f);
                draw_text(batch, item.position.x + 10.0f, item.position.y + 10.0f,
                    context_text ? edit_labels[index] : labels[index], 1.12f,
                    hovered ? text_primary : text_secondary);
            }
        }

        void draw_libraries_page(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "INTERLINKED PUBLIC LIBRARIES", 1.8f, text_primary);
#if EPOCH_DEMO_PLATFORM
            draw_text(batch, 246.0f, 180.0f, "PLATFORM: " + libraries.platform_key, 1.3f, text_secondary);
            draw_text(batch, 246.0f, 208.0f, libraries.native_policy.supports_parented_multiwindow
                ? "NATIVE POLICY: PARENTED WINDOWS SUPPORTED" : "NATIVE POLICY: SINGLE/TOP-LEVEL WINDOWS", 1.15f, text_muted);
            draw_text(batch, 246.0f, 236.0f, EPOCHGUI_DEMO_PLATFORM_HOSTED
                ? "WINDOW AND EVENTS OWNED BY PLATFORM DRIVER" : "PLATFORM POLICY ONLY; NATIVE DEMO HOST", 1.15f, text_muted);
#else
            draw_text(batch, 246.0f, 180.0f, "GUI WITH EXISTING NATIVE DEMO HOST", 1.3f, text_secondary);
#endif
#if EPOCH_DEMO_MEDIA
            draw_text(batch, 246.0f, 286.0f, "MEDIA: GENERATED 1920 X 1080 / 30 FPS METADATA", 1.3f, text_primary);
            draw_text(batch, 246.0f, 314.0f, "NO DECODED VIDEO - TRANSPORT AND FIT GEOMETRY ONLY", 1.15f, text_muted);
#endif
            const gui::Rect step{{246.0f, 350.0f}, {220.0f, 36.0f}};
            const bool hover = gui::contains(step, frame.pointer_position);
            draw_surface(batch, step, true, 7.0f, hover ? control_hover : control_fill, panel_border, 1.0f);
            draw_text(batch, 258.0f, 362.0f, "ADVANCE HALF SECOND", 1.2f, text_secondary);
            if (!context_popup.open && hover && frame.pointer(input::PointerButton::left).released)
                (void)libraries.step();
            const auto progress = libraries.progress({{246.0f, 402.0f}, {600.0f, 16.0f}});
            batch.append_rect(progress.track, control_fill);
            batch.append_rect(progress.fill, accent);
#if EPOCH_DEMO_MEDIA
            draw_text(batch, 246.0f, 438.0f, "PLAYHEAD: " + std::to_string(libraries.timeline.playhead_seconds), 1.2f, text_secondary);
            if (libraries.timeline.source)
            {
                const auto fit = media::timeline_preview::contain_fit(*libraries.timeline.source, {900.0, 286.0, 300.0, 180.0});
                if (fit) batch.append_rect({{static_cast<float>(fit->x), static_cast<float>(fit->y)},
                    {static_cast<float>(fit->width), static_cast<float>(fit->height)}}, panel_border);
                draw_text(batch, 910.0f, 480.0f, "ASPECT FIT GEOMETRY", 1.1f, text_muted);
            }
#endif
#if EPOCH_DEMO_AUDIO
            draw_text(batch, 246.0f, 510.0f, "AUDIO: GENERATED PCM MIX - NO OUTPUT DEVICE", 1.3f, text_primary);
            draw_text(batch, 246.0f, 546.0f, "MIXED FRAMES: " + std::to_string(libraries.mixer.metrics().frames_produced), 1.2f, text_secondary);
            draw_text(batch, 246.0f, 578.0f, "LAST SOURCES: " + std::to_string(libraries.mixed.metrics.mixed_sources)
                + "  SAMPLES: " + std::to_string(libraries.mixed.interleaved_samples.size()), 1.2f, text_secondary);
#endif
            draw_text(batch, 246.0f, 620.0f, libraries.ready ? "LIBRARY MODELS READY" : "LIBRARY MODEL FAILED", 1.25f,
                libraries.ready ? green : warning);
        }

        void draw_showcase(const input::InputFrame& frame)
        {
            draw_text(batch, 246.0f, 140.0f, "WIDGET SHOWCASE", 1.8f, text_primary);
            draw_text(batch, 246.0f, 168.0f, "LIVE CONTROLS TOGETHER - FOLD SECTIONS OR USE THE LINKS ON THE LEFT", 1.15f, text_secondary);
            {
                ClipScope viewport_clip{batch, showcase_viewport()};
                constexpr std::array<std::string_view, 4> labels{
                    "CONTROLS AND LAYOUT", "TEXT EDITING AND SELECTION", "IMAGES AND ASPECT FIT", "INPUT INSPECTOR"};
                for (std::size_t index = 0; index < showcase_open.size(); ++index)
                {
                    const auto header = section_header(index);
                    draw_surface(batch, header, true, 7.0f,
                        gui::contains(header, frame.pointer_position) ? control_hover : control_fill, panel_border, 1.0f);
                    draw_text(batch, header.position.x + 12.0f, header.position.y + 11.0f,
                        std::string(showcase_open[index] ? "-  " : "+  ") + std::string(labels[index]), 1.35f, text_primary);
                    if (!showcase_open[index]) continue;
                    batch.translation.y = header.position.y + 44.0f - 184.0f;
                    {
                        ClipScope body_clip{batch, {{234.0f, 184.0f}, {1008.0f, 600.0f}}};
                        const auto local = section_input(frame, index);
                        switch (index)
                        {
                        case 0: draw_overview(local); break;
                        case 1: draw_text_page(); break;
                        case 2: draw_image_page(local); break;
                        case 3: draw_input_page(local); break;
                        default: break;
                        }
                    }
                    batch.translation = {};
                }
            }
            const auto scroll = showcase_scrollbar();
            batch.append_rect(scroll.track, panel_fill);
            draw_surface(batch, scroll.thumb, true, 5.0f, showcase_scroll_drag ? accent : control_hover, panel_border, 1.0f);
        }

        void build_scene()
        {
            batch.clear();
            batch.pixel_scale = {static_cast<float>(framebuffer_width) / canvas_width,
                                 static_cast<float>(framebuffer_height) / canvas_height};
            batch.vertices.reserve(60000);
            batch.commands.reserve(5000);
            const input::InputFrame& frame = input_tracker.frame();
            batch.append_rect({ { 0.0f, 0.0f }, { canvas_width, canvas_height } }, background);
            draw_custom_title_bar(frame);
            draw_toolbar(frame);
            draw_sidebar();
            batch.append_rect(content_area(), workspace_fill);

            switch (page)
            {
            case 0: draw_showcase(frame); break;
            case 1: draw_windows_page(frame); break;
            case 2: draw_text_page(); break;
            case 3: draw_image_page(frame); break;
            case 4: draw_input_page(frame); break;
            case 5: draw_libraries_page(frame); break;
            default: break;
            }
            draw_context_menu(frame);
        }

        void render()
        {
            if (!initialized)
                return;
            update_ui();
            build_scene();
            gl.viewport(0, 0, framebuffer_width, framebuffer_height);
            gl.clear_color(background.r, background.g, background.b, background.a);
            gl.clear(GL_COLOR_BUFFER_BIT);
            gl.use_program(program);
            gl.uniform_2f(canvas_uniform, canvas_width, canvas_height);
            gl.bind_vertex_array(vertex_array);
            gl.bind_buffer(GL_ARRAY_BUFFER, vertex_buffer);
            gl.buffer_data(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(batch.vertices.size() * sizeof(gui::Vec2)),
                batch.vertices.empty() ? nullptr : batch.vertices.data(),
                GL_DYNAMIC_DRAW);
            std::optional<gui::Rect> applied_clip{};
            for (const DrawCommand& command : batch.commands)
            {
                const bool same_clip = command.clip.has_value() == applied_clip.has_value()
                    && (!command.clip || (command.clip->position.x == applied_clip->position.x
                        && command.clip->position.y == applied_clip->position.y
                        && command.clip->size.x == applied_clip->size.x
                        && command.clip->size.y == applied_clip->size.y));
                if (!same_clip)
                {
                    if (command.clip)
                    {
                        const auto clipped = intersection(*command.clip, {{0.0f, 0.0f}, {canvas_width, canvas_height}});
                        const float sx = static_cast<float>(framebuffer_width) / canvas_width;
                        const float sy = static_cast<float>(framebuffer_height) / canvas_height;
                        const int left = static_cast<int>(std::ceil(clipped.position.x * sx));
                        const int right = static_cast<int>(std::floor((clipped.position.x + clipped.size.x) * sx));
                        const int top = static_cast<int>(std::ceil(clipped.position.y * sy));
                        const int bottom = static_cast<int>(std::floor((clipped.position.y + clipped.size.y) * sy));
                        gl.enable(GL_SCISSOR_TEST);
                        gl.scissor(left, framebuffer_height - bottom, (std::max)(0, right - left), (std::max)(0, bottom - top));
                    }
                    else gl.disable(GL_SCISSOR_TEST);
                    applied_clip = command.clip;
                }
                gl.uniform_4f(color_uniform, command.color.r, command.color.g, command.color.b, command.color.a);
                gl.draw_arrays(GL_TRIANGLES, command.first, command.count);
            }
            gl.disable(GL_SCISSOR_TEST);
            input_tracker.finish_frame();
        }
    };

    OpenGLRenderer::OpenGLRenderer()
        : impl_(std::make_unique<Impl>())
    {
    }

    OpenGLRenderer::~OpenGLRenderer() = default;
    OpenGLRenderer::OpenGLRenderer(OpenGLRenderer&&) noexcept = default;
    OpenGLRenderer& OpenGLRenderer::operator=(OpenGLRenderer&&) noexcept = default;

    bool OpenGLRenderer::initialize(OpenGLProcLoader loader) noexcept
    {
        return impl_ && impl_->initialize(loader);
    }

    void OpenGLRenderer::resize(int width, int height) noexcept
    {
        if (!impl_)
            return;
        impl_->framebuffer_width = (std::max)(1, width);
        impl_->framebuffer_height = (std::max)(1, height);
    }

    void OpenGLRenderer::pointer_move(float x, float y) noexcept
    {
        if (impl_)
            impl_->input_tracker.set_pointer_position(impl_->to_canvas(x, y));
    }

    void OpenGLRenderer::pointer_button(PointerButton button, bool down) noexcept
    {
        if (impl_)
            impl_->input_tracker.set_pointer_button(button, down);
    }

    void OpenGLRenderer::wheel(float horizontal, float vertical) noexcept
    {
        if (impl_)
            impl_->input_tracker.add_wheel_delta({ horizontal, vertical });
    }

    void OpenGLRenderer::key(Key key_code, bool down, bool repeated) noexcept
    {
        if (impl_)
        {
            if (key_code == input::Key::tab && down && !repeated
                && impl_->input_tracker.frame().modifiers.control)
                impl_->pending_workspace_step = impl_->input_tracker.frame().modifiers.shift ? -1 : 1;
            impl_->input_tracker.set_key(key_code, down, repeated);
        }
    }

    void OpenGLRenderer::modifiers(input::ModifierState state) noexcept
    {
        if (impl_)
            impl_->input_tracker.set_modifiers(state);
    }

    void OpenGLRenderer::text_input(std::string_view text)
    {
        if (impl_)
            impl_->pending_text.append(text);
    }

    std::string OpenGLRenderer::edit_command(int command, std::string_view pasted_text)
    {
        return impl_ ? impl_->edit_command(command, pasted_text) : std::string{};
    }

    void OpenGLRenderer::set_clipboard(const char* (*reader)(void*),
        bool (*writer)(void*, const char*), void* user) noexcept
    {
        if (!impl_) return;
        impl_->clipboard_read = reader;
        impl_->clipboard_write = writer;
        impl_->clipboard_user = user;
    }

    void OpenGLRenderer::render() noexcept
    {
        if (impl_)
            impl_->render();
    }

    WindowChromeRegion OpenGLRenderer::window_chrome_hit_test(float x, float y) const noexcept
    {
        if (!impl_)
            return WindowChromeRegion::client;
        return input::hit_test_borderless_window_chrome(impl_->chrome_layout, impl_->to_canvas(x, y));
    }

    WindowCommand OpenGLRenderer::take_window_command() noexcept
    {
        if (!impl_)
            return WindowCommand::none;
        return std::exchange(impl_->pending_window_command, WindowCommand::none);
    }

    int OpenGLRenderer::take_native_frame_mode() noexcept
    {
        if (!impl_)
            return -1;
        return std::exchange(impl_->pending_native_frame_mode, -1);
    }
}
