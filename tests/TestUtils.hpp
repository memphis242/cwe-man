#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>

namespace cweman::test {

inline std::filesystem::path make_temp_dir(const std::string& prefix) {
    auto base = std::filesystem::temp_directory_path();
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<unsigned long long>(now));
    auto suffix = std::to_string(rng());
    auto dir = base / (prefix + "-" + suffix);
    std::filesystem::create_directories(dir);
    return dir;
}

class ScopedHome {
public:
    explicit ScopedHome(std::filesystem::path home)
        : previous_{std::getenv("HOME") ? std::getenv("HOME") : ""}, had_previous_{std::getenv("HOME") != nullptr} {
        std::filesystem::create_directories(home);
        setenv("HOME", home.c_str(), 1);
    }

    ~ScopedHome() {
        if (had_previous_) {
            setenv("HOME", previous_.c_str(), 1);
        } else {
            unsetenv("HOME");
        }
    }

private:
    std::string previous_;
    bool had_previous_{false};
};

} // namespace cweman::test
