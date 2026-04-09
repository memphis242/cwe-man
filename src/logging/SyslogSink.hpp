#pragma once

#include <string_view>
#include <syslog.h>

namespace cweman {

class SyslogSink {
public:
    SyslogSink() {
        openlog("cwe-man", LOG_PID | LOG_CONS, LOG_USER);
    }

    ~SyslogSink() {
        closelog();
    }

    SyslogSink(const SyslogSink&) = delete;
    SyslogSink& operator=(const SyslogSink&) = delete;

    void write(std::string_view message) {
        syslog(LOG_CRIT, "%.*s",
               static_cast<int>(message.size()), message.data());
    }
};

} // namespace cweman
