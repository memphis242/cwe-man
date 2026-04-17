#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace cweman {

class FileSink {
public:
    using DateProvider = std::function<std::string()>;
    using TimestampProvider = std::function<std::string()>;

    explicit FileSink(const std::filesystem::path& log_dir);
    FileSink(const std::filesystem::path& log_dir,
             DateProvider date_provider,
             TimestampProvider timestamp_provider);
    ~FileSink();

    FileSink(const FileSink&) = delete;
    FileSink& operator=(const FileSink&) = delete;

    void write(std::string_view level, std::string_view message);

private:
    void rotate_if_needed();
    std::filesystem::path log_path_for_today() const;

    std::filesystem::path log_dir_;
    std::string           current_date_;
    std::ofstream         stream_;
    std::mutex            mutex_;
    DateProvider          date_provider_;
    TimestampProvider     timestamp_provider_;
};

} // namespace cweman
