#pragma once

#include <memory>
#include <string_view>

namespace cweman {

class FileSink;
class SyslogSink;

class Logger {
public:
    static Logger& instance();

    void debug(std::string_view msg);
    void info(std::string_view msg);
    void warn(std::string_view msg);
    void error(std::string_view msg);
    void critical(std::string_view msg);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    std::unique_ptr<FileSink>   file_sink_;
    std::unique_ptr<SyslogSink> syslog_sink_;
};

} // namespace cweman
