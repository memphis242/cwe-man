#pragma once

#include "data/Models.hpp"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cweman {

enum class AppMode {
    Tree,
    Detail,
    Notification,
    Command,
    Search,
    Filter,
};

struct AppState {
    // Tree data
    std::vector<Category>  categories;
    std::vector<TreeNode>  visible_nodes;    // flattened tree for rendering

    // Selection
    int  cursor_index{0};                    // index into visible_nodes
    std::set<int> expanded_categories;       // category IDs currently expanded

    // Currently viewed CWE (when in Detail mode)
    std::optional<Cwe> active_cwe;

    // Pane visibility
    bool tree_visible{true};
    bool notification_pane_visible{false};

    // Mode
    AppMode mode{AppMode::Tree};

    // Search
    std::string search_query;
    bool search_case_sensitive{false};
    std::vector<int> search_matches;      // indices into visible_nodes
    std::vector<std::pair<size_t, size_t>> search_match_pos;  // (start, length) for each match
    int search_match_idx{-1};             // current match in search_matches

    // Filter
    std::string filter_query;
    bool filter_case_sensitive{false};
    std::vector<int> filter_matches;      // indices into all CWEs that match
    bool filter_active{false};            // true while filtered tree view should stay active
    bool filter_navigation_active{false}; // true after Enter in filter mode

    // Command
    std::string command_input;
    std::vector<std::string> completions;  // autocomplete candidates
    int completion_idx{-1};                // selected completion (-1 = none)

    // Notifications
    std::vector<Notification> notifications;
    int notification_index{0};

    // Popup overlay (for command-driven info windows)
    bool popup_visible{false};
    std::string popup_title;
    std::vector<std::string> popup_lines;
    bool popup_solid_bg{false};

    // Vim-style pending prefix count for list-like movement.
    bool count_pending{false};
    int  count_value{0};
    std::string last_key_sequence;

    // Status line message
    std::string status_message;
    bool        db_empty{true};
};

} // namespace cweman
