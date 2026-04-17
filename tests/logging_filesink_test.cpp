#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "logging/FileSink.hpp"

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
    std::vector<std::string> dates{
        "2026-04-14", // constructor
        "2026-04-14", // first write (no rotation)
        "2026-04-15"  // second write (rotation)
    };
    std::size_t date_index = 0;
    auto date_provider = [&]() {
        if (date_index < dates.size()) {
            return dates[date_index++];
        }
        return dates.back();
    };

    std::vector<std::string> timestamps{
        "2026-04-14 10:00:00.000 CDT",
        "2026-04-15 11:11:11.111 CDT"
    };
    std::size_t timestamp_index = 0;
    auto timestamp_provider = [&]() {
        if (timestamp_index < timestamps.size()) {
            return timestamps[timestamp_index++];
        }
        return timestamps.back();
    };

    FileSink sink(log_dir, date_provider, timestamp_provider);

    sink.write("INFO", "before rotate");
    sink.write("WARN", "after rotate");

    const auto day1_path = log_dir / "cwe-man-2026-04-14.log";
    const auto day2_path = log_dir / "cwe-man-2026-04-15.log";
    ASSERT_TRUE(std::filesystem::exists(day1_path));
    ASSERT_TRUE(std::filesystem::exists(day2_path));

    const auto day1 = read_file(day1_path);
    const auto day2 = read_file(day2_path);
    EXPECT_NE(day1.find("2026-04-14 10:00:00.000 CDT [INFO] before rotate"), std::string::npos);
    EXPECT_EQ(day1.find("after rotate"), std::string::npos);
    EXPECT_NE(day2.find("2026-04-15 11:11:11.111 CDT [WARN] after rotate"), std::string::npos);
}

TEST(FileSinkTest, AppendsMultipleEntriesToSameDayFile) {
    auto log_dir = make_temp_dir();
    auto date_provider = []() {
        return std::string{"2026-04-16"};
    };
    std::vector<std::string> timestamps{
        "2026-04-16 12:00:00.123 CDT",
        "2026-04-16 12:00:01.456 CDT"
    };
    std::size_t timestamp_index = 0;
    auto timestamp_provider = [&]() {
        if (timestamp_index < timestamps.size()) {
            return timestamps[timestamp_index++];
        }
        return timestamps.back();
    };

    FileSink sink(log_dir, date_provider, timestamp_provider);
    sink.write("ERROR", "first");
    sink.write("CRITICAL", "second");

    const auto path = log_dir / "cwe-man-2026-04-16.log";
    ASSERT_TRUE(std::filesystem::exists(path));

    const auto content = read_file(path);
    EXPECT_NE(content.find("2026-04-16 12:00:00.123 CDT [ERROR] first"), std::string::npos);
    EXPECT_NE(content.find("2026-04-16 12:00:01.456 CDT [CRITICAL] second"), std::string::npos);

    const auto first_pos = content.find("[ERROR] first");
    const auto second_pos = content.find("[CRITICAL] second");
    ASSERT_NE(first_pos, std::string::npos);
    ASSERT_NE(second_pos, std::string::npos);
    EXPECT_LT(first_pos, second_pos);
}

} // namespace cweman
