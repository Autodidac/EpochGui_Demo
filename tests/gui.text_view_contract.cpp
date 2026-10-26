// SPDX-License-Identifier: MIT
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
import epoch.gui;
import gui.text_view;

namespace gui = epochengine::gui_lib;
namespace view = epochengine::gui_demo::text_view;
static bool near(float a, float b) { return std::abs(a - b) < 0.01f; }

int main()
{
    // Long lines wrap through the library's byte-safe visual line owner.
    gui::TextControlState state{};
    state.text = "abcdefghijklmnopqrstuvwxyz";
    state.caret = state.text.size();
    auto layout = view::layout(state, {80.0f, 40.0f}, 1.0f, true);
    if (layout.lines.size() != 3 || layout.lines[0].past_last != 10
        || layout.lines[1].first != 10 || !near(layout.metrics.caret_position.x, 36.0f)
        || !near(layout.metrics.caret_position.y, 18.0f)) return 1;

    // Hit testing uses the same wrapped rows; click after scrolling must not
    // jump back to the first hard newline or select half a UTF-8 character.
    state.scroll = {0.0f, 9.0f};
    layout = view::layout(state, {80.0f, 40.0f}, 1.0f, true);
    if (view::caret_from_point(state, layout, {22.0f, 10.0f}) != 12
        || view::caret_from_point(state, layout, {-50.0f, 10.0f}) != 10) return 2;

    state.text = "a\xc3\xa9\xf0\x9f\x98\x80";
    state.caret = state.text.size();
    state.scroll = {};
    layout = view::layout(state, {26.0f, 80.0f}, 1.0f, true);
    if (layout.lines.size() != 3 || layout.lines[1].first != 1
        || layout.lines[1].past_last != 3 || layout.lines[2].first != 3
        || view::caret_from_point(state, layout, {10.0f, 20.0f}) != 1) return 3;
    if (view::glyph(state.text, 1, 1.0f).byte_count != 2
        || view::glyph(state.text, 3, 1.0f).byte_count != 4
        || !near(view::width(state.text, 0, state.text.size(), 1.0f), 18.0f)) return 4;

    // Four-cell tabs must agree in wrapping, hit mapping and visible width.
    state.text = "a\tb";
    layout = view::layout(state, {200.0f, 50.0f}, 1.0f, false);
    if (!near(layout.metrics.content_size.x, 36.0f)
        || !near(view::glyph(state.text, 1, 1.0f).advance, 24.0f)) return 5;

    // Wheel scrolling is bounded independently of the caret. The text controller
    // owns edit/undo/caret movement; this view never rewrites the text document.
    state.text = std::string(400, 'x');
    state.caret = 0;
    layout = view::layout(state, {80.0f, 45.0f}, 1.0f, true);
    view::scroll_by(state, layout, {0.0f, 100000.0f});
    if (!(state.scroll.y > 0) || !near(state.scroll.y, layout.maximum_scroll.y)
        || state.caret != 0 || state.text.size() != 400) return 6;
    view::scroll_by(state, layout, {0.0f, -100000.0f});
    if (state.scroll.y != 0) return 7;
    view::scroll_by(state, layout, {0.0f, std::numeric_limits<float>::quiet_NaN()});
    if (state.scroll.y != 0) return 8;

    // Selection crosses a soft wrap and is returned inside control-local space.
    state.text = "abcdefghijkl";
    state.anchor = 8;
    state.caret = 12;
    layout = view::layout(state, {80.0f, 80.0f}, 1.0f, true);
    const auto rectangles = view::selection_rectangles(state, layout);
    if (rectangles.size() != 2 || !near(rectangles[0].position.x, 58.0f)
        || !near(rectangles[0].size.x, 12.0f) || !near(rectangles[1].position.y, 19.0f)
        || !near(rectangles[1].size.x, 12.0f)) return 9;

    // Empty/trailing newline rows and malformed UTF-8 never make invalid ranges.
    state.text = "\n";
    state.caret = 1;
    layout = view::layout(state, {80.0f, 80.0f}, 1.0f, true);
    if (layout.lines.size() != 2 || !near(layout.metrics.caret_position.y, 9.0f)) return 10;
    if (view::glyph("\xe0\x80\x80", 0, 1.0f).byte_count != 1
        || view::glyph("\xf0", 0, 1.0f).byte_count != 1) return 11;
    state.text.clear();
    layout = view::layout(state, {0.0f, 0.0f}, 0.0f, true);
    if (layout.lines.size() != 1 || !layout.metrics.valid
        || view::caret_from_point(state, layout, {}) != 0) return 12;
    return 0;
}
