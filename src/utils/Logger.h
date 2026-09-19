#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <chrono>

namespace GamepadReceiver {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    static void log(LogLevel level, const std::string& message) {
        if (level < s_minLevel) return;

        std::lock_guard<std::mutex> lock(s_mutex);
        const char* prefix = "[INFO]";
        switch (level) {
            case LogLevel::Trace: prefix = "[TRACE]"; break;
            case LogLevel::Debug: prefix = "[DEBUG]"; break;
            case LogLevel::Info:  prefix = "[INFO] "; break;
            case LogLevel::Warn:  prefix = "[WARN] "; break;
            case LogLevel::Error: prefix = "[ERROR]"; break;
        }
        std::cout << prefix << " " << message << std::endl;
    }

    static void setLevel(LogLevel level) { s_minLevel = level; }

private:
    static inline LogLevel s_minLevel = LogLevel::Info;
    static inline std::mutex s_mutex;
};

#define LOG_TRACE(msg) GamepadReceiver::Logger::log(GamepadReceiver::LogLevel::Trace, msg)
#define LOG_DEBUG(msg) GamepadReceiver::Logger::log(GamepadReceiver::LogLevel::Debug, msg)
#define LOG_INFO(msg)  GamepadReceiver::Logger::log(GamepadReceiver::LogLevel::Info, msg)
#define LOG_WARN(msg)  GamepadReceiver::Logger::log(GamepadReceiver::LogLevel::Warn, msg)
#define LOG_ERROR(msg) GamepadReceiver::Logger::log(GamepadReceiver::LogLevel::Error, msg)

} // namespace GamepadReceiver
