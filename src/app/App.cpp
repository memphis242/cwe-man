#include "app/App.hpp"
#include "api/Sync.hpp"
#include "logging/Logger.hpp"
#include "ui/Layout.hpp"
#include "ui/TreePane.hpp"

#include <cstdlib>
#include <fstream>
#include <format>
#include <string_view>
#include <thread>
#include <algorithm>
#include <cctype>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace cweman {

namespace {

std::string trim_copy(std::string s) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto left = std::find_if_not(s.begin(), s.end(), is_space);
    auto right = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    if (left >= right) return {};
    return std::string(left, right);
}

std::string first_line(std::string text) {
    size_t end = text.find('\n');
    if (end != std::string::npos) {
        text = text.substr(0, end);
    }
    if (!text.empty() && text.back() == '\r') {
        text.pop_back();
    }
    return trim_copy(std::move(text));
}

} // namespace

App::App() {
    auto dir = data_dir();
    auto db_path = dir / "cwe.db";

    db_   = std::make_unique<Database>(db_path);
    repo_ = std::make_unique<Repository>(*db_);
    repo_->initialize_schema();

    load_data();
}

void App::load_data() {
    state_.db_empty = !repo_->has_data();
    if (!state_.db_empty) {
        state_.categories = repo_->get_categories();
        rebuild_visible_nodes(state_, *repo_);
        Logger::instance().info(
            std::format("Loaded {} categories", state_.categories.size()));
    } else {
        Logger::instance().info("Database is empty; user needs to run :sync");
    }
}

void App::run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    // Provide a way for Layout to trigger sync and quit
    auto on_command = [&](const std::string& cmd) {
        if (cmd == "q" || cmd == "quit") {
            screen.Exit();
        } else if (cmd == "sync") {
            start_sync(screen);
        } else if (cmd == "print-cwes" || cmd.starts_with("print-cwes ")) {
            std::string filename;
            if (cmd.size() > std::string_view{"print-cwes"}.size()) {
                filename = trim_copy(cmd.substr(std::string_view{"print-cwes"}.size()));
            }
            if (filename.empty()) {
                filename = "cwe-list.md";
            }

            int rows = 0;
            std::string error;
            if (print_cwes_markdown(filename, rows, error)) {
                state_.status_message =
                    std::format("Printed {} rows to {}", rows, filename);
            } else {
                state_.status_message =
                    std::format("print-cwes failed: {}", error);
            }
        } else if (cmd.starts_with("cwe ")) {
            auto arg = cmd.substr(4);
            // Try parsing as an integer ID
            try {
                int id = std::stoi(arg);
                auto cwe = repo_->get_cwe(id);
                if (cwe) {
                    state_.active_cwe = std::move(*cwe);
                    state_.mode = AppMode::Detail;
                    state_.status_message.clear();
                } else {
                    state_.status_message =
                        std::format("CWE-{} not found", id);
                }
            } catch (...) {
                state_.status_message =
                    std::format("Invalid CWE ID: {}", arg);
            }
        } else {
            state_.status_message = std::format("Unknown command: {}", cmd);
        }
    };

    auto layout = RootLayout(state_, *repo_, on_command);

    auto root = CatchEvent(layout, [&](Event event) -> bool {
        // ESC clears status/command and returns to tree
        if (event == Event::Escape) {
            if (state_.mode == AppMode::Command) {
                state_.mode = AppMode::Tree;
                state_.command_input.clear();
                return true;
            }
            state_.status_message.clear();
            if (state_.mode == AppMode::Detail) {
                state_.mode = AppMode::Tree;
                state_.active_cwe.reset();
                return true;
            }
            return true;
        }

        return false;
    });

    Logger::instance().info("Starting cwe-man UI");

    // Check if sync is needed on startup
    if (!state_.db_empty && Sync::sync_needed()) {
        state_.status_message = "Data may be stale. Run :sync to update.";
    }

    screen.Loop(root);
    Logger::instance().info("cwe-man exiting");
}

bool App::print_cwes_markdown(const std::filesystem::path& path, int& written_rows,
                              std::string& error) {
    written_rows = 0;
    error.clear();

    auto rows = repo_->get_cwes_for_print();
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        error = std::format("cannot open {}", path.string());
        return false;
    }

    out << "# CWE List\n\n";
    out << "_Generated by cwe-man from local database._\n\n";

    std::string current_category;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.category_name != current_category) {
            current_category = row.category_name;
            out << "## " << current_category << "\n\n";
        }

        std::string summary = first_line(row.description);
        if (summary.empty()) {
            summary = "(no summary)";
        }

        out << "- CWE-" << row.id << ": " << row.name << " - " << summary;
        if (!row.url.empty()) {
            out << " ([link](" << row.url << "))";
        }
        bool last_in_category =
            (i + 1 == rows.size()) || (rows[i + 1].category_name != row.category_name);
        out << (last_in_category ? "\n\n" : "\n");
        ++written_rows;
    }

    if (!out.good()) {
        error = std::format("failed writing {}", path.string());
        return false;
    }
    return true;
}

void App::start_sync(ftxui::ScreenInteractive& screen) {
    if (sync_running_) {
        state_.status_message = "Sync already in progress...";
        return;
    }

    sync_running_ = true;
    state_.status_message = "Syncing...";

    std::thread([this, &screen] {
        Sync sync{*repo_};

        sync.run(699,
            // Progress callback
            [this, &screen](const std::string& msg, int step, int total) {
                screen.Post([this, msg, step, total] {
                    if (total > 0) {
                        state_.status_message =
                            std::format("Sync: {} ({}/{})", msg, step, total);
                    } else {
                        state_.status_message = std::format("Sync: {}", msg);
                    }
                });
                screen.RequestAnimationFrame();
            },
            // Complete callback
            [this, &screen](bool success, const std::string& msg) {
                screen.Post([this, success, msg] {
                    state_.status_message = msg;
                    if (success) {
                        load_data();
                    }
                    sync_running_ = false;
                });
                screen.RequestAnimationFrame();
            }
        );
    }).detach();
}

std::filesystem::path App::data_dir() const {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::filesystem::path{home} / ".cwe-man";
}

} // namespace cweman
