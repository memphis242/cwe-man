#include "ui/Layout.hpp"
#include "ui/TreePane.hpp"
#include "ui/DetailPane.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <memory>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "logging/Logger.hpp"

namespace cweman {

namespace {

bool icontains(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

void update_search_matches(AppState& state) {
    state.search_matches.clear();
    state.search_match_idx = -1;
    if (state.search_query.empty()) return;

    for (int i = 0; i < static_cast<int>(state.visible_nodes.size()); ++i) {
        const auto& node = state.visible_nodes[i];
        if (icontains(node.label, state.search_query)) {
            state.search_matches.push_back(i);
        } else if (node.kind == TreeNode::Kind::Category &&
                   icontains(std::to_string(node.id), state.search_query)) {
            state.search_matches.push_back(i);
        }
    }

    for (int mi = 0; mi < static_cast<int>(state.search_matches.size()); ++mi) {
        if (state.search_matches[mi] >= state.cursor_index) {
            state.search_match_idx = mi;
            state.cursor_index = state.search_matches[mi];
            return;
        }
    }
    if (!state.search_matches.empty()) {
        state.search_match_idx = 0;
        state.cursor_index = state.search_matches[0];
    }
}

void jump_to_next_match(AppState& state) {
    if (state.search_matches.empty()) return;
    state.search_match_idx =
        (state.search_match_idx + 1) %
        static_cast<int>(state.search_matches.size());
    state.cursor_index = state.search_matches[state.search_match_idx];
}

void jump_to_prev_match(AppState& state) {
    if (state.search_matches.empty()) return;
    state.search_match_idx =
        (state.search_match_idx - 1 +
         static_cast<int>(state.search_matches.size())) %
        static_cast<int>(state.search_matches.size());
    state.cursor_index = state.search_matches[state.search_match_idx];
}

void update_completions(AppState& state, Repository& repo) {
    state.completions.clear();
    state.completion_idx = -1;

    auto input = state.command_input;
    if (input.find(' ') == std::string::npos) {
        for (const auto& c : {"q", "quit", "sync", "cwe "}) {
            std::string cmd{c};
            if (cmd.starts_with(input)) {
                state.completions.push_back(cmd);
            }
        }
        return;
    }
    if (input.starts_with("cwe ")) {
        auto arg = input.substr(4);
        if (arg.empty()) return;
        auto matches = repo.search_cwes(arg);
        for (const auto& cwe : matches) {
            state.completions.push_back(std::format("cwe {}", cwe.id));
        }
    }
}

} // namespace

ftxui::Component RootLayout(AppState& state, Repository& repo,
                            CommandCallback on_command) {
    using namespace ftxui;

    auto detail_scroll = std::make_shared<int>(0);

    auto component = Renderer([&, detail_scroll] {
        // ── Build main content area ─────────────────────────────────
        Element main_content;

        bool tree_focused = (state.mode == AppMode::Tree ||
                             state.mode == AppMode::Search);
        bool detail_focused = (state.mode == AppMode::Detail);

        Element tree_el = RenderTreePane(state, tree_focused);
        Element detail_el = RenderDetailPane(state, *detail_scroll, detail_focused);

        if (state.tree_visible) {
            main_content = hbox({
                tree_el | size(WIDTH, EQUAL, 42),
                detail_el | flex,
            });
        } else {
            main_content = detail_el | flex;
        }

        // ── Bottom bar ──────────────────────────────────────────────
        Element bottom_bar;
        if (state.mode == AppMode::Command) {
            Elements cmd_line = {
                text(":") | bold,
                text(state.command_input),
                text("_") | blink,
            };
            if (state.completion_idx >= 0 &&
                state.completion_idx < static_cast<int>(state.completions.size())) {
                auto hint = state.completions[state.completion_idx];
                if (hint.size() > state.command_input.size()) {
                    cmd_line.push_back(
                        text(hint.substr(state.command_input.size())) | dim);
                }
            }
            if (!state.completions.empty() && state.completions.size() <= 10) {
                Elements comps;
                for (int i = 0; i < static_cast<int>(state.completions.size()); ++i) {
                    auto el = text(" " + state.completions[i] + " ");
                    if (i == state.completion_idx) el = el | inverted;
                    comps.push_back(el);
                }
                bottom_bar = vbox({
                    hbox(std::move(comps)),
                    hbox(std::move(cmd_line)),
                });
            } else {
                if (state.completions.size() > 10) {
                    cmd_line.push_back(
                        text(std::format(" ({} matches)",
                                         state.completions.size())) | dim);
                }
                bottom_bar = hbox(std::move(cmd_line));
            }
        } else if (state.mode == AppMode::Search) {
            std::string match_info;
            if (!state.search_query.empty()) {
                match_info = state.search_matches.empty()
                    ? " (no matches)"
                    : std::format(" ({}/{})",
                          state.search_match_idx + 1,
                          state.search_matches.size());
            }
            bottom_bar = hbox({
                text("/") | bold,
                text(state.search_query),
                text("_") | blink,
                text(match_info) | dim,
            });
        } else if (!state.status_message.empty()) {
            bottom_bar = text(" " + state.status_message) | dim;
        } else {
            std::string mode_str;
            switch (state.mode) {
                case AppMode::Tree:    mode_str = "TREE"; break;
                case AppMode::Detail:  mode_str = "DETAIL"; break;
                case AppMode::Command: mode_str = "COMMAND"; break;
                case AppMode::Search:  mode_str = "SEARCH"; break;
            }
            std::string extra;
            if (!state.search_matches.empty() &&
                state.mode == AppMode::Tree) {
                extra = std::format(" | n/N: {}/{}",
                    state.search_match_idx + 1,
                    state.search_matches.size());
            }
            bottom_bar = text(" " + mode_str + extra + " | :q to quit")
                         | dim | inverted;
        }

        return vbox({
            main_content | flex,
            bottom_bar,
        });
    });

    component |= CatchEvent([&, detail_scroll](Event event) -> bool {
        if (event.is_character()) {
            Logger::instance().debug(std::format(
                "Event: char='{}' mode={}", event.character(),
                static_cast<int>(state.mode)));
        }
        // ── Search mode ─────────────────────────────────────────────
        if (state.mode == AppMode::Search) {
            if (event == Event::Return) {
                state.mode = AppMode::Tree;
                if (!state.search_matches.empty()) {
                    state.status_message = std::format(
                        "/{} ({} matches)", state.search_query,
                        state.search_matches.size());
                }
                return true;
            }
            if (event == Event::Escape) {
                state.search_query.clear();
                state.search_matches.clear();
                state.search_match_idx = -1;
                state.mode = AppMode::Tree;
                return true;
            }
            if (event == Event::Backspace) {
                if (!state.search_query.empty()) {
                    state.search_query.pop_back();
                    update_search_matches(state);
                }
                return true;
            }
            if (event.is_character()) {
                state.search_query += event.character();
                update_search_matches(state);
                return true;
            }
            return true;
        }

        // ── Command mode ────────────────────────────────────────────
        if (state.mode == AppMode::Command) {
            if (event == Event::Return) {
                if (state.completion_idx >= 0 &&
                    state.completion_idx < static_cast<int>(state.completions.size())) {
                    state.command_input = state.completions[state.completion_idx];
                }
                auto cmd = state.command_input;
                state.command_input.clear();
                state.completions.clear();
                state.completion_idx = -1;
                state.mode = AppMode::Tree;
                if (on_command && !cmd.empty()) on_command(cmd);
                return true;
            }
            if (event == Event::Escape) {
                state.command_input.clear();
                state.completions.clear();
                state.completion_idx = -1;
                state.mode = AppMode::Tree;
                return true;
            }
            if (event == Event::Tab) {
                if (state.completions.empty()) update_completions(state, repo);
                if (!state.completions.empty()) {
                    state.completion_idx =
                        (state.completion_idx + 1) %
                        static_cast<int>(state.completions.size());
                    state.command_input = state.completions[state.completion_idx];
                }
                return true;
            }
            if (event == Event::TabReverse) {
                if (!state.completions.empty()) {
                    state.completion_idx =
                        (state.completion_idx - 1 +
                         static_cast<int>(state.completions.size())) %
                        static_cast<int>(state.completions.size());
                    state.command_input = state.completions[state.completion_idx];
                }
                return true;
            }
            if (event == Event::Backspace) {
                if (!state.command_input.empty()) {
                    state.command_input.pop_back();
                    update_completions(state, repo);
                }
                return true;
            }
            if (event.is_character()) {
                state.command_input += event.character();
                state.completion_idx = -1;
                update_completions(state, repo);
                return true;
            }
            return true;
        }

        // ── Detail mode ─────────────────────────────────────────────
        if (state.mode == AppMode::Detail) {
            if (event == Event::Character(':')) {
                state.mode = AppMode::Command;
                state.command_input.clear();
                state.status_message.clear();
                return true;
            }
            if (event == Event::Character('q')) {
                state.mode = AppMode::Tree;
                state.active_cwe.reset();
                *detail_scroll = 0;
                return true;
            }
            if (event == Event::Character('a')) {
                state.mode = AppMode::Tree;  // keep active_cwe open
                return true;
            }
            if (event == Event::Character('j') || event == Event::ArrowDown) {
                ++(*detail_scroll);
                return true;
            }
            if (event == Event::Character('k') || event == Event::ArrowUp) {
                *detail_scroll = std::max(0, *detail_scroll - 1);
                return true;
            }
            if (event == Event::Character('G')) {
                *detail_scroll = 9999;
                return true;
            }
            if (event == Event::Character('g')) {
                *detail_scroll = 0;
                return true;
            }
            // Ctrl+D — page down
            if (event == Event::Special({4})) {
                *detail_scroll += 15;
                return true;
            }
            // Ctrl+U — page up
            if (event == Event::Special({21})) {
                *detail_scroll = std::max(0, *detail_scroll - 15);
                return true;
            }
        }

        // ── Tree mode ───────────────────────────────────────────────
        if (state.mode == AppMode::Tree && !state.visible_nodes.empty()) {
            int max_idx = static_cast<int>(state.visible_nodes.size()) - 1;

            if (event == Event::Character('d') && state.active_cwe) {
                state.mode = AppMode::Detail;
                return true;
            }
            if (event == Event::Character('j') || event == Event::ArrowDown) {
                state.cursor_index = std::min(state.cursor_index + 1, max_idx);
                return true;
            }
            if (event == Event::Character('k') || event == Event::ArrowUp) {
                state.cursor_index = std::max(state.cursor_index - 1, 0);
                return true;
            }
            if (event == Event::Character('{')) {
                for (int i = state.cursor_index - 1; i >= 0; --i) {
                    if (state.visible_nodes[i].kind == TreeNode::Kind::Category) {
                        state.cursor_index = i;
                        break;
                    }
                }
                return true;
            }
            if (event == Event::Character('}')) {
                for (int i = state.cursor_index + 1; i <= max_idx; ++i) {
                    if (state.visible_nodes[i].kind == TreeNode::Kind::Category) {
                        state.cursor_index = i;
                        break;
                    }
                }
                return true;
            }
            if (event == Event::Character('o') || event == Event::Return) {
                const auto& node = state.visible_nodes[state.cursor_index];
                if (node.kind == TreeNode::Kind::Category) {
                    if (state.expanded_categories.contains(node.id))
                        state.expanded_categories.erase(node.id);
                    else
                        state.expanded_categories.insert(node.id);
                    rebuild_visible_nodes(state, repo);
                } else {
                    auto cwe = repo.get_cwe(node.id);
                    if (cwe) {
                        state.active_cwe = std::move(*cwe);
                        state.mode = AppMode::Detail;
                        *detail_scroll = 0;
                    }
                }
                return true;
            }
            if (event == Event::Character('x')) {
                const auto& node = state.visible_nodes[state.cursor_index];
                int cat_id = (node.kind == TreeNode::Kind::Category)
                                 ? node.id : node.category_id;
                state.expanded_categories.erase(cat_id);
                rebuild_visible_nodes(state, repo);
                for (int i = 0; i < static_cast<int>(state.visible_nodes.size()); ++i) {
                    if (state.visible_nodes[i].kind == TreeNode::Kind::Category &&
                        state.visible_nodes[i].id == cat_id) {
                        state.cursor_index = i;
                        break;
                    }
                }
                return true;
            }
            if (event == Event::Character('h')) {
                const auto& node = state.visible_nodes[state.cursor_index];
                if (node.kind == TreeNode::Kind::Category &&
                    state.expanded_categories.contains(node.id)) {
                    state.expanded_categories.erase(node.id);
                    rebuild_visible_nodes(state, repo);
                } else if (node.kind == TreeNode::Kind::Weakness) {
                    for (int i = 0; i < static_cast<int>(state.visible_nodes.size()); ++i) {
                        if (state.visible_nodes[i].kind == TreeNode::Kind::Category &&
                            state.visible_nodes[i].id == node.category_id) {
                            state.cursor_index = i;
                            break;
                        }
                    }
                }
                return true;
            }
            if (event == Event::Character('l')) {
                const auto& node = state.visible_nodes[state.cursor_index];
                if (node.kind == TreeNode::Kind::Category &&
                    !state.expanded_categories.contains(node.id)) {
                    state.expanded_categories.insert(node.id);
                    rebuild_visible_nodes(state, repo);
                } else if (node.kind == TreeNode::Kind::Weakness) {
                    auto cwe = repo.get_cwe(node.id);
                    if (cwe) {
                        state.active_cwe = std::move(*cwe);
                        state.mode = AppMode::Detail;
                        *detail_scroll = 0;
                    }
                }
                return true;
            }
            if (event == Event::Character('G')) {
                state.cursor_index = max_idx;
                return true;
            }
            if (event == Event::Character('g')) {
                state.cursor_index = 0;
                return true;
            }
        }

        // ── Global keys (any mode except Command/Search) ────────────

        // / — enter search mode
        if (event == Event::Character('/') && state.mode == AppMode::Tree) {
            state.mode = AppMode::Search;
            state.search_query.clear();
            state.search_matches.clear();
            state.search_match_idx = -1;
            state.status_message.clear();
            return true;
        }

        // n/N — cycle search matches
        if (event == Event::Character('n') && state.mode == AppMode::Tree) {
            jump_to_next_match(state);
            return true;
        }
        if (event == Event::Character('N') && state.mode == AppMode::Tree) {
            jump_to_prev_match(state);
            return true;
        }

        // : — enter command mode
        if (event == Event::Character(':') &&
            (state.mode == AppMode::Tree || state.mode == AppMode::Detail)) {
            state.mode = AppMode::Command;
            state.command_input.clear();
            state.status_message.clear();
            return true;
        }

        // Ctrl+T — toggle tree pane
        if (event == Event::CtrlT) {
            state.tree_visible = !state.tree_visible;
            return true;
        }

        // Ctrl+N — toggle notification pane (placeholder)
        if (event == Event::CtrlN) {
            state.notification_pane_visible = !state.notification_pane_visible;
            state.status_message = state.notification_pane_visible
                ? "Notification pane (not yet implemented)"
                : "";
            return true;
        }

        // ESC — context-dependent dismiss
        if (event == Event::Escape) {
            state.status_message.clear();
            if (state.mode == AppMode::Detail) {
                state.mode = AppMode::Tree;
                state.active_cwe.reset();
                *detail_scroll = 0;
                return true;
            }
            return true;
        }

        return false;
    });

    return component;
}

} // namespace cweman
