#include "logger.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// Singleton accessor - first call creates the instance, all subsequent
// calls return the same one. This is standard embedded pattern for
// shared resources.
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Opens harmony.log in append mode so we don't lose logs on restart
    log_file_.open("harmony.log", std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "WARNING: Could not open harmony.log for writing\n";
    }
    info("LOGGER", "Logger initialized");
}

Logger::~Logger() {
    info("LOGGER", "Logger shutting down");
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log(LogLevel level, const std::string& category, const std::string& message) {
    // Lock so watchdog thread and main thread don't interleave log lines
    std::lock_guard<std::mutex> lock(mutex_);

    std::string entry = "[" + current_timestamp() + "] "
                      + level_to_string(level) + " "
                      + category + ": "
                      + message;

    // Write to both file and stdout so you can see it in the terminal
    if (log_file_.is_open()) {
        log_file_ << entry << "\n";
        log_file_.flush();  // Flush immediately - if daemon crashes we want the log intact
    }
    std::cout << entry << "\n";
}

void Logger::info(const std::string& category, const std::string& message) {
    log(LogLevel::INFO, category, message);
}

void Logger::warn(const std::string& category, const std::string& message) {
    log(LogLevel::WARNING, category, message);
}

void Logger::error(const std::string& category, const std::string& message) {
    log(LogLevel::ERROR, category, message);
}

std::string Logger::current_timestamp() {
    // Format: 2024-01-15 14:23:07 - matches the spec exactly
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:    return "INFO   ";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR  ";
        default:                return "UNKNOWN";
    }
}