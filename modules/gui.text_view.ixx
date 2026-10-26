// SPDX-License-Identifier: MIT
module;
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
export module gui.text_view;
import epoch.gui;

export namespace epochengine::gui_demo::text_view
{
    namespace gui = epochengine::gui_lib;
    inline constexpr float padding = 10.0f;

    struct Layout
    {
        std::vector<gui::TextVisualLine> lines{};
        gui::TextControlMetrics metrics{};
        gui::Vec2 viewport{};
        gui::Vec2 text_origin{};
        gui::Vec2 maximum_scroll{};
        float scale{1.6f};
        float line_height{14.4f};
        bool multiline{true};
    };

    // The tiny bitmap font displays one '?' for an unsupported Unicode scalar,
    // so layout and drawing must consume that scalar as one glyph, not its bytes.
    [[nodiscard]] inline gui::TextGlyphMetrics glyph(
        std::string_view text, std::size_t index, float scale) noexcept
    {
        if (index >= text.size()) return {};
        const auto first = static_cast<unsigned char>(text[index]);
        std::size_t count = 1;
        if (first >= 0xc2 && first <= 0xdf) count = 2;
        else if (first >= 0xe0 && first <= 0xef) count = 3;
        else if (first >= 0xf0 && first <= 0xf4) count = 4;
        if (count > text.size() - index) count = 1;
        for (std::size_t byte = 1; byte < count; ++byte)
            if ((static_cast<unsigned char>(text[index + byte]) & 0xc0u) != 0x80u)
            { count = 1; break; }
        if (count > 1)
        {
            const auto second = static_cast<unsigned char>(text[index + 1]);
            if ((first == 0xe0 && second < 0xa0) || (first == 0xed && second >= 0xa0)
                || (first == 0xf0 && second < 0x90) || (first == 0xf4 && second >= 0x90))
                count = 1;
        }
        const float advance = first == '\n' || first == '\r' ? 0.0f
            : (first == '\t' ? 24.0f : 6.0f) * scale;
        return {count, advance, first == ' ' || first == '\t'};
    }

    [[nodiscard]] inline float width(std::string_view text, std::size_t first,
        std::size_t past_last, float scale) noexcept
    {
        past_last = (std::min)(past_last, text.size());
        float result{};
        for (std::size_t index = (std::min)(first, past_last); index < past_last;)
        {
            const auto measured = glyph(text, index, scale);
            if (measured.byte_count > past_last - index) break;
            result += measured.advance;
            index += (std::max)(std::size_t{1}, measured.byte_count);
        }
        return result;
    }

    [[nodiscard]] inline gui::TextGlyphMetrics measure(
        std::string_view text, std::size_t index, const void* context) noexcept
    {
        return glyph(text, index, *static_cast<const float*>(context));
    }

    [[nodiscard]] inline Layout layout(const gui::TextControlState& state,
        gui::Vec2 viewport_size, float scale, bool multiline)
    {
        Layout result{};
        result.scale = std::isfinite(scale) && scale > 0.0f ? scale : 1.6f;
        result.line_height = 9.0f * result.scale;
        result.multiline = multiline;
        result.viewport = viewport_size;
        result.text_origin = {padding - state.scroll.x, padding - state.scroll.y};
        const gui::TextLayoutMetrics measure_options{measure, &result.scale, result.scale * 6.0f};
        const float inner_width = (std::max)(1.0f, viewport_size.x - padding * 2.0f);
        result.lines = gui::text_visual_lines(state.text, inner_width, multiline, measure_options);
        float widest{};
        for (const auto& line : result.lines)
            widest = (std::max)(widest, width(state.text, line.first, line.past_last, result.scale));
        const std::size_t caret = (std::min)(state.caret, state.text.size());
        const auto row = gui::text_visual_line_for_index(result.lines, caret);
        result.metrics = {
            .content_size = {widest, static_cast<float>(result.lines.size()) * result.line_height},
            .caret_position = {width(state.text, result.lines[row].first, caret, result.scale),
                static_cast<float>(row) * result.line_height},
            .caret_size = {result.scale, 7.0f * result.scale},
            .valid = true
        };
        const auto horizontal = gui::make_scrollbar_layout({
            .track = {{}, {1.0f, (std::max)(0.0f, viewport_size.x - 2.0f * padding)}},
            .content_extent = widest});
        const auto vertical = gui::make_scrollbar_layout({
            .track = {{}, {1.0f, (std::max)(0.0f, viewport_size.y - 2.0f * padding)}},
            .content_extent = result.metrics.content_size.y});
        result.maximum_scroll = {horizontal.maximum_scroll_offset,
            multiline ? vertical.maximum_scroll_offset : 0.0f};
        return result;
    }

    [[nodiscard]] inline std::size_t caret_from_point(const gui::TextControlState& state,
        const Layout& view, gui::Vec2 local_point) noexcept
    {
        const gui::TextLayoutMetrics measure_options{measure, &view.scale, view.scale * 6.0f};
        return gui::text_index_from_point(state.text, view.lines,
            {local_point.x - padding + state.scroll.x, local_point.y - padding + state.scroll.y},
            view.line_height, view.multiline, measure_options);
    }

    inline void scroll_by(gui::TextControlState& state, const Layout& view, gui::Vec2 delta) noexcept
    {
        const auto clamp_axis = [](float offset, float change, float maximum)
        {
            const float next = offset + change;
            return std::isfinite(next) ? (std::clamp)(next, 0.0f, maximum) : 0.0f;
        };
        state.scroll = {clamp_axis(state.scroll.x, delta.x, view.maximum_scroll.x),
            clamp_axis(state.scroll.y, delta.y, view.maximum_scroll.y)};
    }

    // Rectangles are control-local, including padding and the current scroll.
    // The renderer clips these and all glyph/caret drawing to the text viewport.
    [[nodiscard]] inline std::vector<gui::Rect> selection_rectangles(
        const gui::TextControlState& state, const Layout& view)
    {
        std::vector<gui::Rect> rectangles;
        const auto selected = gui::text_selection(state);
        for (std::size_t row = 0; row < view.lines.size(); ++row)
        {
            const auto& line = view.lines[row];
            const auto first = (std::max)(line.first, selected.first);
            const auto end = (std::min)(line.past_last, selected.past_last);
            if (end <= first) continue;
            const float left = width(state.text, line.first, first, view.scale);
            rectangles.push_back({
                {padding - state.scroll.x + left,
                    padding - state.scroll.y + static_cast<float>(row) * view.line_height},
                {width(state.text, first, end, view.scale), view.line_height}});
        }
        return rectangles;
    }
}
