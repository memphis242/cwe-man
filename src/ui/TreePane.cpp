#include "ui/TreePane.hpp"
#include "logging/Logger.hpp"

#include <algorithm>
#include <format>

#include <ftxui/dom/elements.hpp>

namespace cweman {

void rebuild_visible_nodes(AppState& state, Repository& repo) {
    state.visible_nodes.clear();

    for (const auto& cat : state.categories) {
        bool expanded = state.expanded_categories.contains(cat.id);

        auto cwes = repo.get_cwes_for_category(cat.id);
        int cwe_count = static_cast<int>(cwes.size());

        state.visible_nodes.push_back(TreeNode{
            .kind        = TreeNode::Kind::Category,
            .id          = cat.id,
            .label       = std::format("{} ({})", cat.name, cwe_count),
            .category_id = cat.id,
            .expanded    = expanded,
        });

        if (expanded) {
            for (const auto& cwe : cwes) {
                state.visible_nodes.push_back(TreeNode{
                    .kind        = TreeNode::Kind::Weakness,
                    .id          = cwe.id,
                    .label       = std::format("CWE-{}: {}", cwe.id, cwe.name),
                    .category_id = cat.id,
                    .expanded    = false,
                });
            }
        }
    }

    if (!state.visible_nodes.empty()) {
        state.cursor_index = std::clamp(
            state.cursor_index, 0,
            static_cast<int>(state.visible_nodes.size()) - 1);
    } else {
        state.cursor_index = 0;
    }
}

ftxui::Element RenderTreePane(AppState& state, bool focused) {
    using namespace ftxui;

    Elements lines;

    if (state.db_empty) {
        lines.push_back(text(""));
        lines.push_back(text(" No CWE data loaded.") | dim);
        lines.push_back(text(""));
        lines.push_back(text(" Run :sync to fetch data.") | bold);
        auto w = window(text(" CWE Tree ") | bold,
                        vbox(std::move(lines)) | flex);
        return focused ? w : (w | dim);
    }

    // In active filter view, only show matching CWE entries.
    if (state.filter_active) {
        for (int i : state.filter_matches) {
            const auto& node = state.visible_nodes[i];
            if (node.kind != TreeNode::Kind::Weakness) continue;

            std::string full_label = "    " + node.label;
            auto line = text(full_label) | dim;
            if (i == state.cursor_index) {
                line = text(full_label) | inverted | focus;
            }
            lines.push_back(line);
        }

        if (lines.empty()) {
            lines.push_back(text(" (no matches)") | dim);
        }

        auto title = text(" CWE Tree ") | bold;
        auto w = window(title,
                        vbox(std::move(lines)) | vscroll_indicator | yframe | flex);
        return focused ? w : (w | dim);
    }

    // Normal tree rendering (not in filter mode)
    for (int i = 0; i < static_cast<int>(state.visible_nodes.size()); ++i) {
        const auto& node = state.visible_nodes[i];
        Element line;

        // Check if this node is a search match and get match position
        auto match_it = std::find(state.search_matches.begin(),
                                  state.search_matches.end(), i);
        bool is_search_match = (match_it != state.search_matches.end());
        std::pair<size_t, size_t> match_pos{0, 0};
        if (is_search_match) {
            size_t match_idx = match_it - state.search_matches.begin();
            if (match_idx < state.search_match_pos.size()) {
                match_pos = state.search_match_pos[match_idx];
            }
        }

        if (node.kind == TreeNode::Kind::Category) {
            std::string prefix = node.expanded ? " ▼ " : " ▶ ";
            std::string full_label = prefix + node.label;

            if (is_search_match && match_pos.second > 0) {
                // Highlight the matching substring
                size_t start = match_pos.first;
                size_t len = match_pos.second;
                if (start < node.label.size()) {
                    start += prefix.size();  // Account for prefix
                    Elements parts;
                    if (start > 0)
                        parts.push_back(text(full_label.substr(0, start)) | bold);
                    if (start + len <= full_label.size())
                        parts.push_back(text(full_label.substr(start, len)) | bold | color(Color::Yellow));
                    if (start + len < full_label.size())
                        parts.push_back(text(full_label.substr(start + len)) | bold);
                    line = hbox(std::move(parts));
                } else {
                    line = text(full_label) | bold;
                }
            } else {
                line = text(full_label) | bold;
            }
        } else {
            std::string full_label = "    " + node.label;

            if (is_search_match) {
                // For weakness entries, highlight the whole line to avoid spacing issues
                line = text(full_label) | dim | color(Color::Yellow);
            } else {
                line = text(full_label) | dim;
            }
        }

        if (i == state.cursor_index) {
            if (node.kind == TreeNode::Kind::Category) {
                line = text((node.expanded ? " ▼ " : " ▶ ") + node.label)
                       | bold | inverted | focus;
            } else {
                line = text("    " + node.label) | inverted | focus;
            }
        }

        lines.push_back(line);
    }

    if (lines.empty()) {
        lines.push_back(text(" (empty)") | dim);
    }

    auto title = text(" CWE Tree ") | bold;
    auto w = window(title,
                    vbox(std::move(lines)) | vscroll_indicator | yframe | flex);
    return focused ? w : (w | dim);
}

} // namespace cweman
