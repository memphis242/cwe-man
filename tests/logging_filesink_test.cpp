#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <sstream>

#define private public
#include "logging/FileSink.hpp"
#undef private

#include <gtest/gtest.h>

namespace cweman {
namespace {

std::filesystem::path make_temp_dir() {
    auto base = std::filesystem::temp_directory_path();
    auto seed = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937_64 rng(seed);
    auto dir = base / ("cwe-man-filesink-test-" + std::to_string(rng()));
    std::filesystem::create_directories(dir);
    return dir;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    return content;
}

} // namespace

TEST(FileSinkTest, RotatesWhenCurrentDateIsStale) {
    auto log_dir = make_temp_dir();
    FileSink sink(log_dir);

    sink.write("INFO", "before rotate");

    // Force stale date branch inside rotate_if_needed().
    sink.current_date_ = "1900-01-01";

    sink.write("INFO", "after rotate");

    EXPECT_NE(sink.current_date_, "1900-01-01");

    auto active_path = sink.log_path_for_today();
    EXPECT_TRUE(std::filesystem::exists(active_path));

    const auto content = read_file(active_path);
    EXPECT_NE(content.find("after rotate"), std::string::npos);
}

} // namespace cweman
