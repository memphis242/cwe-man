#pragma once

#include "app/State.hpp"
#include "data/Database.hpp"
#include "data/Repository.hpp"

#include <atomic>
#include <filesystem>
#include <memory>

#include <ftxui/component/screen_interactive.hpp>

namespace cweman {

class App {
public:
    App();
    void run();

private:
    std::filesystem::path data_dir() const;
    void load_data();
    void start_sync(ftxui::ScreenInteractive& screen);
    bool print_cwes_markdown(const std::filesystem::path& path, int& written_rows,
                             std::string& error);

    AppState                    state_;
    std::unique_ptr<Database>   db_;
    std::unique_ptr<Repository> repo_;
    std::atomic<bool>           sync_running_{false};
};

} // namespace cweman
