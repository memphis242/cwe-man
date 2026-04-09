#pragma once

#include "app/State.hpp"

#include <ftxui/dom/elements.hpp>

namespace cweman {

// Render the detail pane as an Element (not a Component).
// scroll_pos is clamped to the valid range on each call.
ftxui::Element RenderDetailPane(AppState& state, int& scroll_pos, bool focused);

} // namespace cweman
