#include "logging/Logger.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace cweman {
namespace {

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

TEST(LoggerTest, FallsBackToTmpWhenHomeIsUnset) {
    ASSERT_EQ(unsetenv("HOME"), 0);

    auto& logger = Logger::instance();
    const auto unique_suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto message = "logger no home " + unique_suffix;
    logger.info(message);

    const auto log_dir = std::filesystem::path{"/tmp"} / ".cwe-man" / "logs";
    ASSERT_TRUE(std::filesystem::exists(log_dir));

    const auto log_file = find_log_file(log_dir);
    ASSERT_FALSE(log_file.empty());

    const auto content = read_file(log_file);
    EXPECT_NE(content.find("[INFO] " + message), std::string::npos);
}

} // namespace
} // namespace cweman
