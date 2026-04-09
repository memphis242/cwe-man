#include "ui/DetailPane.hpp"

#include <algorithm>
#include <format>

#include <ftxui/dom/elements.hpp>

namespace cweman {

ftxui::Element RenderDetailPane(AppState& state, int& scroll_pos, bool focused) {
    using namespace ftxui;

    if (!state.active_cwe) {
        auto w = window(text(" Detail ") | bold, vbox({
            text(""),
            text(" No CWE selected.") | dim,
            text(""),
            text(" Select a CWE from the tree and") | dim,
            text(" press 'o' or 'l' to view.") | dim,
        }) | flex);
        return focused ? (w | color(Color::Cyan)) : w;
    }

    const auto& cwe = *state.active_cwe;
    Elements content;

    auto add_line = [&](Element el) {
        content.push_back(el);
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
        add_line(paragraph(" " + cwe.description));
    }

    // Extended description
    if (!cwe.extended_description.empty()) {
        add_line(text(""));
        add_line(text(" Extended Description") | bold | underlined);
        add_line(paragraph(" " + cwe.extended_description));
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
            add_line(hbox({text("  Scope: ") | bold, text(scope_str) | flex}));
            add_line(hbox({text("  Impact: ") | bold, text(impact_str) | flex}));
            if (!con.note.empty()) {
                add_line(paragraph("  " + con.note));
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
                add_line(hbox({text("  Phase: ") | bold, text(mit.phase)}));
            }
            if (!mit.strategy.empty()) {
                add_line(hbox({text("  Strategy: ") | bold, text(mit.strategy)}));
            }
            if (!mit.description.empty()) {
                add_line(paragraph("  " + mit.description));
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
        add_line(paragraph("  " + ids));
    }

    // URL
    if (!cwe.url.empty()) {
        add_line(text(""));
        add_line(hbox({text(" URL: ") | bold, text(cwe.url) | underlined}));
    }

    add_line(text(""));
    add_line(text(" q: close | a: tree | j/k: scroll | g/G: top/bottom") | dim);

    // Clamp scroll_pos in place so callers can't over-scroll
    int max_scroll = std::max(0, static_cast<int>(content.size()) - 1);
    scroll_pos = std::clamp(scroll_pos, 0, max_scroll);

    // Build scrolled view by dropping lines before scroll_pos
    Elements visible;
    for (int i = scroll_pos; i < static_cast<int>(content.size()); ++i) {
        visible.push_back(std::move(content[i]));
    }

    auto title = text(std::format(" CWE-{} ", cwe.id)) | bold;
    auto w = window(title, vbox(std::move(visible)) | flex);
    return focused ? (w | color(Color::Cyan)) : w;
}

} // namespace cweman
