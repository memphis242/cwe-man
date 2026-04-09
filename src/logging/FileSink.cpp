#include "logging/FileSink.hpp"

#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace cweman {

namespace {

std::string today_string() {
    auto now  = std::chrono::system_clock::now();
    auto days = std::chrono::floor<std::chrono::days>(now);
    std::chrono::year_month_day ymd{days};
    return std::format("{:%Y-%m-%d}", ymd);
}

std::string timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace

FileSink::FileSink(const std::filesystem::path& log_dir)
    : log_dir_{log_dir}
{
    std::filesystem::create_directories(log_dir_);
    current_date_ = today_string();
    stream_.open(log_path_for_today(), std::ios::app);
}

FileSink::~FileSink() {
    if (stream_.is_open()) {
        stream_.close();
    }
}

void FileSink::write(std::string_view level, std::string_view message) {
    std::lock_guard lock{mutex_};
    rotate_if_needed();
    if (stream_.is_open()) {
        stream_ << timestamp_string() << " [" << level << "] " << message << '\n';
        stream_.flush();
    }
}

void FileSink::rotate_if_needed() {
    auto today = today_string();
    if (today != current_date_) {
        stream_.close();
        current_date_ = today;
        stream_.open(log_path_for_today(), std::ios::app);
    }
}

std::filesystem::path FileSink::log_path_for_today() const {
    return log_dir_ / ("cwe-man-" + current_date_ + ".log");
}

} // namespace cweman
