#include "ui/DetailPane.hpp"

#include <algorithm>
#include <format>

#include <sys/ioctl.h>
#include <unistd.h>

#include <ftxui/dom/elements.hpp>

namespace cweman {

namespace {

// Word-wrap a string into individual lines of at most `width` characters.
// Breaks at word boundaries. Preserves leading whitespace on all lines.
std::vector<std::string> wrap_text(const std::string& input, int width) {
    std::vector<std::string> lines;
    if (width <= 0 || input.empty()) {
        lines.push_back(input);
        return lines;
    }

    // Extract leading whitespace
    size_t first_non_space = input.find_first_not_of(' ');
    std::string indent;
    std::string text_content;
    if (first_non_space == std::string::npos) {
        indent = input;
        text_content = "";
    } else {
        indent = input.substr(0, first_non_space);
        text_content = input.substr(first_non_space);
    }

    std::string remaining = text_content;
    while (!remaining.empty()) {
        if (static_cast<int>(remaining.size()) <= width - static_cast<int>(indent.size())) {
            lines.push_back(indent + remaining);
            break;
        }

        // Find last space within available width (accounting for indent)
        int avail_width = width - static_cast<int>(indent.size());
        int break_at = avail_width;
        for (int i = avail_width; i > 0; --i) {
            if (remaining[i] == ' ') {
                break_at = i;
                break;
            }
        }

        lines.push_back(indent + remaining.substr(0, break_at));
        // Skip the space at the break point
        size_t next = break_at;
        if (next < remaining.size() && remaining[next] == ' ') ++next;
        remaining = remaining.substr(next);
    }

    return lines;
}

} // namespace

ftxui::Element RenderDetailPane(AppState& state, int& scroll_pos, bool focused) {
    using namespace ftxui;

    if (!state.active_cwe) {
        auto w = window(text(""), vbox({
            text(""),
            text(" No CWE selected.") | dim,
            text(""),
            text(" Select a CWE from the tree and") | dim,
            text(" press 'o' or 'l' to view.") | dim,
        }) | flex);
        return focused ? w : (w | dim);
    }

    // Compute available text width from terminal size
    struct winsize ws{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int term_width = ws.ws_col > 0 ? ws.ws_col : 80;
    int pane_width = term_width - (state.tree_visible ? 42 : 0);
    int text_width = std::max(20, pane_width - 4); // subtract window border + padding

    const auto& cwe = *state.active_cwe;
    Elements content;

    auto add_line = [&](Element el) {
        content.push_back(el);
    };

    // Word-wrap a string and add each line as a text() element
    auto add_wrapped = [&](const std::string& str) {
        for (const auto& line : wrap_text(str, text_width)) {
            add_line(text(line));
        }
    };

    // Header
    add_line(text(std::format(" CWE-{}: {}", cwe.id, cwe.name)) | bold);
    add_line(separator());

    // Metadata
    if (!cwe.abstraction.empty()) {
        add_line(hbox({text(" Abstraction: ") | bold,
                       text(cwe.abstraction)}));
    }
    if (!cwe.status.empty()) {
        add_line(hbox({text(" Status: ") | bold, text(cwe.status)}));
    }
    if (!cwe.likelihood_of_exploit.empty()) {
        add_line(hbox({text(" Likelihood: ") | bold,
                       text(cwe.likelihood_of_exploit)}));
    }

    // Description
    if (!cwe.description.empty()) {
        add_line(text(""));
        add_line(text(" Description") | bold | underlined);
        add_wrapped(" " + cwe.description);
    }

    // Extended description
    if (!cwe.extended_description.empty()) {
        add_line(text(""));
        add_line(text(" Extended Description") | bold | underlined);
        add_wrapped(" " + cwe.extended_description);
    }

    // Consequences
    if (!cwe.consequences.empty()) {
        add_line(text(""));
        add_line(text(" Consequences") | bold | underlined);
        for (size_t ci = 0; ci < cwe.consequences.size(); ++ci) {
            const auto& con = cwe.consequences[ci];
            std::string scope_str;
            for (const auto& s : con.scopes) {
                if (!scope_str.empty()) scope_str += ", ";
                scope_str += s;
            }
            std::string impact_str;
            for (const auto& s : con.impacts) {
                if (!impact_str.empty()) impact_str += ", ";
                impact_str += s;
            }
            if (!scope_str.empty()) {
                add_line(hbox({text(" Scope:") | bold, paragraph(" " + scope_str) | flex}));
            }
            if (!impact_str.empty()) {
                add_line(hbox({text(" Impact:") | bold, paragraph(" " + impact_str) | flex}));
            }
            if (!con.note.empty()) {
                add_wrapped(" " + con.note);
            }
            if (ci + 1 < cwe.consequences.size()) {
                add_line(text(""));
            }
        }
    }

    // Mitigations
    if (!cwe.mitigations.empty()) {
        add_line(text(""));
        add_line(text(" Mitigations") | bold | underlined);
        for (size_t mi = 0; mi < cwe.mitigations.size(); ++mi) {
            const auto& mit = cwe.mitigations[mi];
            if (!mit.phase.empty()) {
                add_line(hbox({text(" Phase: ") | bold, text(mit.phase)}));
            }
            if (!mit.strategy.empty()) {
                add_line(hbox({text(" Strategy: ") | bold, text(mit.strategy)}));
            }
            if (!mit.description.empty()) {
                add_wrapped(" " + mit.description);
            }
            if (mi + 1 < cwe.mitigations.size()) {
                add_line(separatorLight());
            }
        }
    }

    // Related weaknesses
    if (!cwe.related_weakness_ids.empty()) {
        add_line(text(""));
        add_line(text(" Related Weaknesses") | bold | underlined);
        std::string ids;
        for (int rid : cwe.related_weakness_ids) {
            if (!ids.empty()) ids += ", ";
            ids += std::format("CWE-{}", rid);
        }
        add_wrapped(" " + ids);
    }

    // URL
    if (!cwe.url.empty()) {
        add_line(text(""));
        add_line(hbox({text(" URL: ") | bold, text(cwe.url) | underlined}));
    }

    // Compute max scroll: allow scrolling until last line is at bottom of pane
    int term_height = ws.ws_row > 0 ? ws.ws_row : 24;
    int visible_lines = std::max(1, term_height - 5); // border(2) + footer(1) + status bar(1) + margin(1)
    int max_scroll = std::max(0, static_cast<int>(content.size()) - visible_lines);
    scroll_pos = std::clamp(scroll_pos, 0, max_scroll);

    // Build scrolled view by dropping lines before scroll_pos
    Elements visible;
    for (int i = scroll_pos; i < static_cast<int>(content.size()); ++i) {
        visible.push_back(std::move(content[i]));
    }

    auto footer = text(" q: close | h: tree | j/k: scroll | g/G: top/bottom") | dim;

    auto w = window(text(""), vbox({vbox(std::move(visible)) | flex, footer}));
    return focused ? w : (w | dim);
}

} // namespace cweman
