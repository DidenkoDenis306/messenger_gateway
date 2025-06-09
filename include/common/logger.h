#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger final {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    static void log(Level level, const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void debug(const std::string& message);

private:
    static std::string get_current_time();
    static std::string level_to_string(Level level);
};

#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARNING(msg) Logger::warning(msg)
#define LOG_ERROR(msg) Logger::error(msg)
#define LOG_DEBUG(msg) Logger::debug(msg)