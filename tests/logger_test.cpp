#include "logging/Logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace cweman {
namespace {

std::filesystem::path make_temp_home() {
    auto base = std::filesystem::temp_directory_path();
    auto seed = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937_64 rng(seed);
    auto dir = base / ("cwe-man-logger-test-home-" + std::to_string(rng()));
    std::filesystem::create_directories(dir);
    return dir;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

std::filesystem::path find_log_file(const std::filesystem::path& log_dir) {
    std::filesystem::path newest;
    std::filesystem::file_time_type newest_time{};
    for (const auto& entry : std::filesystem::directory_iterator(log_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto name = entry.path().filename().string();
        if (!name.starts_with("cwe-man-") || entry.path().extension() != ".log") {
            continue;
        }
        const auto write_time = entry.last_write_time();
        if (newest.empty() || write_time > newest_time) {
            newest = entry.path();
            newest_time = write_time;
        }
    }
    return newest;
}

TEST(LoggerTest, WritesAllLevelsAndUsesSingletonInstance) {
    const auto home = make_temp_home();
    ASSERT_EQ(setenv("HOME", home.c_str(), 1), 0);

    auto& logger1 = Logger::instance();
    auto& logger2 = Logger::instance();
    EXPECT_EQ(&logger1, &logger2);

    const auto unique_suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());

    const auto debug_msg    = "logger test debug " + unique_suffix;
    const auto info_msg     = "logger test info " + unique_suffix;
    const auto warn_msg     = "logger test warn " + unique_suffix;
    const auto error_msg    = "logger test error " + unique_suffix;
    const auto critical_msg = "logger test critical " + unique_suffix;

    logger1.debug(debug_msg);
    logger1.info(info_msg);
    logger1.warn(warn_msg);
    logger1.error(error_msg);
    logger1.critical(critical_msg);

    const auto log_dir = home / ".cwe-man" / "logs";
    ASSERT_TRUE(std::filesystem::exists(log_dir));

    const auto log_file = find_log_file(log_dir);
    ASSERT_FALSE(log_file.empty());

    const auto content = read_file(log_file);
    EXPECT_NE(content.find("[DEBUG] " + debug_msg), std::string::npos);
    EXPECT_NE(content.find("[INFO] " + info_msg), std::string::npos);
    EXPECT_NE(content.find("[WARN] " + warn_msg), std::string::npos);
    EXPECT_NE(content.find("[ERROR] " + error_msg), std::string::npos);
    EXPECT_NE(content.find("[CRITICAL] " + critical_msg), std::string::npos);
}

} // namespace
} // namespace cweman
