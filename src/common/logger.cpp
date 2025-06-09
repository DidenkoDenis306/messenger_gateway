#include "common/logger.h"

#include <string>

void Logger::log(Level level, const std::string& message) {
    std::cout << "[" << get_current_time() << "] "
           << "[" << level_to_string(level) << "] "
           << message << std::endl;

}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(Level::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

std::string Logger::get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::INFO:
            return "INFO";
        case Level::WARNING:
            return "WARNING";
        case Level::ERROR:
            return "ERROR";
        default: return "UNKNOWN";
    }
}
