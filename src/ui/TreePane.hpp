#pragma once

#include "app/State.hpp"
#include "data/Repository.hpp"

#include <ftxui/dom/elements.hpp>

namespace cweman {

// Render the tree pane as an Element (not a Component).
// If in search mode, highlights matching substrings with a different color than selection.
ftxui::Element RenderTreePane(AppState& state, bool focused);

// Rebuild the flat visible_nodes list from categories + expanded set.
void rebuild_visible_nodes(AppState& state, Repository& repo);

} // namespace cweman
