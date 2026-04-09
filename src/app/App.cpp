#include "app/App.hpp"
#include "api/Sync.hpp"
#include "logging/Logger.hpp"
#include "ui/Layout.hpp"
#include "ui/TreePane.hpp"

#include <cstdlib>
#include <format>
#include <thread>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace cweman {

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
