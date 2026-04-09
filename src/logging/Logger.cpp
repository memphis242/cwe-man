#include "logging/Logger.hpp"
#include "logging/FileSink.hpp"
#include "logging/SyslogSink.hpp"

#include <cstdlib>
#include <filesystem>

namespace cweman {

namespace {

std::filesystem::path log_directory() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    return std::filesystem::path{home} / ".cwe-man" / "logs";
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger()
    : file_sink_{std::make_unique<FileSink>(log_directory())}
    , syslog_sink_{std::make_unique<SyslogSink>()}
{}

Logger::~Logger() = default;

void Logger::debug(std::string_view msg) {
    file_sink_->write("DEBUG", msg);
}

void Logger::info(std::string_view msg) {
    file_sink_->write("INFO", msg);
}

void Logger::warn(std::string_view msg) {
    file_sink_->write("WARN", msg);
}

void Logger::error(std::string_view msg) {
    file_sink_->write("ERROR", msg);
}

void Logger::critical(std::string_view msg) {
    file_sink_->write("CRITICAL", msg);
    syslog_sink_->write(msg);
}

} // namespace cweman
