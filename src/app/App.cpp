#include "app/App.hpp"
#include "api/Sync.hpp"
#include "logging/Logger.hpp"
#include "ui/Layout.hpp"
#include "ui/TreePane.hpp"

#include <cstdlib>
#include <fstream>
#include <format>
#include <sstream>
#include <string_view>
#include <thread>
#include <algorithm>
#include <cctype>
#include <filesystem>

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
    config_ = load_config();

    load_data();
}

void App::load_data() {
    state_.db_empty = !repo_->has_data();
    state_.notifications = repo_->get_notifications();
    if (!state_.notifications.empty()) {
        state_.notification_index = std::clamp(
            state_.notification_index, 0,
            static_cast<int>(state_.notifications.size()) - 1);
    } else {
        state_.notification_index = 0;
    }
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
        auto input = trim_copy(cmd);
        if (clear_runtime_confirm_pending_) {
            if (input == "yes") {
                int deleted = 0;
                std::string error;
                if (clear_runtime_files(deleted, error)) {
                    state_.status_message =
                        std::format("clear-runtime: deleted {} runtime entries", deleted);
                } else {
                    state_.status_message =
                        std::format("clear-runtime failed: {}", error);
                }
                clear_runtime_confirm_pending_ = false;
                return;
            }
            if (input == "no") {
                clear_runtime_confirm_pending_ = false;
                state_.status_message = "clear-runtime canceled";
                return;
            }
            state_.status_message = "clear-runtime pending: type :yes or :no";
            return;
        }

        if (input == "q" || input == "quit") {
            screen.Exit();
        } else if (input == "sync") {
            start_sync(screen);
        } else if (input == "print-cwes" || input.starts_with("print-cwes ")) {
            std::string filename;
            if (input.size() > std::string_view{"print-cwes"}.size()) {
                filename = trim_copy(input.substr(std::string_view{"print-cwes"}.size()));
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
        } else if (input == "show-config") {
            show_config_popup();
        } else if (input == "rest-api") {
            show_rest_api_popup();
        } else if (input == "clear-runtime") {
            clear_runtime_confirm_pending_ = true;
            state_.status_message = "Confirm clear-runtime: type :yes or :no";
        } else if (input.starts_with("cwe ")) {
            auto arg = input.substr(4);
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
            state_.status_message = std::format("Unknown command: {}", input);
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
    if (!state_.db_empty && config_.auto_sync_enabled &&
        Sync::sync_needed(config_.auto_sync_interval_days)) {
        start_sync(screen);
    }

    screen.Loop(root);
    Logger::instance().info("cwe-man exiting");
}

App::Config App::load_config() {
    Config cfg;
    auto path = config_path();
    std::ifstream in(path);
    if (!in.is_open()) {
        return cfg;
    }

    std::string line;
    while (std::getline(in, line)) {
        auto cleaned = trim_copy(line);
        if (cleaned.empty() || cleaned[0] == '#' || cleaned[0] == ';' ||
            cleaned[0] == '[') {
            continue;
        }
        auto eq = cleaned.find('=');
        if (eq == std::string::npos) continue;
        auto key = trim_copy(cleaned.substr(0, eq));
        auto value = trim_copy(cleaned.substr(eq + 1));

        if (key == "auto_sync_enabled") {
            std::string lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            cfg.auto_sync_enabled =
                (lower == "1" || lower == "true" || lower == "yes" || lower == "on");
        } else if (key == "auto_sync_interval_days") {
            try {
                int days = std::stoi(value);
                cfg.auto_sync_interval_days = std::clamp(days, 1, 3650);
            } catch (...) {
                // Keep default.
            }
        }
    }
    return cfg;
}

std::filesystem::path App::config_path() const {
    return data_dir() / "config.ini";
}

void App::show_config_popup() {
    auto sync_ts = Sync::read_sync_timestamp();
    if (sync_ts.empty()) sync_ts = "(never)";

    state_.popup_title = "Configuration";
    state_.popup_lines = {
        std::format("Config path: {}", config_path().string()),
        std::format("Auto sync enabled: {}", config_.auto_sync_enabled ? "true" : "false"),
        std::format("Auto sync interval days: {}", config_.auto_sync_interval_days),
        std::format("Last sync timestamp: {}", sync_ts),
        std::format("Database path: {}", (data_dir() / "cwe.db").string()),
        std::format("Log path: {}", (data_dir() / "logs").string()),
        "",
        "Press Esc or q to close.",
    };
    state_.popup_solid_bg = false;
    state_.popup_visible = true;
}

void App::show_rest_api_popup() {
    state_.popup_title = "CWE REST API Reminder";
    state_.popup_lines = {
        "Docs: https://github.com/CWE-CAPEC/REST-API-wg/blob/main/Quick%20Start.md",
        "Base URL: https://cwe-api.mitre.org/api/v1",
        "",
        "Endpoints used by cwe-man:",
        "- GET /cwe/view/{id}",
        "- GET /cwe/category/{id}",
        "- GET /cwe/weakness/{id_or_csv_ids}",
        "",
        "Press Esc or q to close.",
    };
    state_.popup_solid_bg = true;
    state_.popup_visible = true;
}

bool App::clear_runtime_files(int& deleted_count, std::string& error) {
    deleted_count = 0;
    error.clear();
    const auto root = data_dir();
    try {
        if (!std::filesystem::exists(root)) {
            return true;
        }

        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            const auto name = entry.path().filename().string();
            bool should_delete_dir = (name == "logs" || name == "tmp" ||
                                      name == "temp" || name == "cache");
            bool should_delete_file = (!entry.is_directory() &&
                                       entry.path().extension() == ".log");

            if (entry.is_directory() && should_delete_dir) {
                deleted_count += static_cast<int>(std::filesystem::remove_all(entry.path()));
            } else if (should_delete_file) {
                if (std::filesystem::remove(entry.path())) {
                    ++deleted_count;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
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
