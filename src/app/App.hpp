#pragma once

#include "app/State.hpp"
#include "data/Database.hpp"
#include "data/Repository.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

#include <ftxui/component/screen_interactive.hpp>

namespace cweman {

class App {
public:
    App();
    void run();

private:
    struct Config {
        bool auto_sync_enabled{true};
        int  auto_sync_interval_days{30};
    };

    std::filesystem::path data_dir() const;
    std::filesystem::path config_path() const;
    Config load_config();
    void load_data();
    void start_sync(ftxui::ScreenInteractive& screen);
    void show_config_popup();
    void show_rest_api_popup();
    bool clear_runtime_files(int& deleted_count, std::string& error);
    bool print_cwes_markdown(const std::filesystem::path& path, int& written_rows,
                             std::string& error);

    AppState                    state_;
    std::unique_ptr<Database>   db_;
    std::unique_ptr<Repository> repo_;
    Config                      config_{};
    std::atomic<bool>           sync_running_{false};
    bool                        clear_runtime_confirm_pending_{false};
};

} // namespace cweman
