#include "logging/Logger.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <string>

namespace {

std::filesystem::path make_temp_home() {
    auto base = std::filesystem::temp_directory_path();
    auto seed = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937_64 rng(seed);
    auto dir = base / ("cwe-man-logger-home-" + std::to_string(rng()));
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

int main(int argc, char** argv) {
    bool unset_home = (argc > 1 && std::string(argv[1]) == "--unset-home");

    std::filesystem::path expected_root;
    if (unset_home) {
        unsetenv("HOME");
        expected_root = std::filesystem::path{"/tmp"} / ".cwe-man" / "logs";
    } else {
        auto home = make_temp_home();
        setenv("HOME", home.c_str(), 1);
        expected_root = home / ".cwe-man" / "logs";
    }

    auto& logger1 = cweman::Logger::instance();
    logger1.debug("logger probe debug");
    logger1.info("logger probe info");
    logger1.warn("logger probe warn");
    logger1.error("logger probe error");
    logger1.critical("logger probe critical");

    // Hit the static-guard "already initialized" path in the same process.
    auto& logger2 = cweman::Logger::instance();
    logger2.info("logger probe second instance call");

    if (!std::filesystem::exists(expected_root)) {
        return 2;
    }

    return 0;
}
