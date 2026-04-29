#pragma once

#include <string>
#include <fstream>
#include <mutex>

// Log categories - each component uses its own so you can grep the log file
enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Singleton - one logger shared across the whole daemon
    static Logger& instance();

    void log(LogLevel level, const std::string& category, const std::string& message);

    // Convenience wrappers so code reads cleanly
    void info(const std::string& category, const std::string& message);
    void warn(const std::string& category, const std::string& message);
    void error(const std::string& category, const std::string& message);

private:
    Logger();  // Private constructor - use instance() instead
    ~Logger();

    std::ofstream log_file_;
    std::mutex mutex_;  // Thread-safe - watchdog and main thread both log

    std::string current_timestamp();
    std::string level_to_string(LogLevel level);
};