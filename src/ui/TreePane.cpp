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

    for (int i = 0; i < static_cast<int>(state.visible_nodes.size()); ++i) {
        const auto& node = state.visible_nodes[i];
        Element line;

        if (node.kind == TreeNode::Kind::Category) {
            std::string prefix = node.expanded ? " ▼ " : " ▶ ";
            line = text(prefix + node.label) | bold;
        } else {
            line = text("    " + node.label) | dim;
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
