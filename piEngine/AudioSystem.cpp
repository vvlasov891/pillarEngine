#pragma once
#include <string>
#include <format>
#include <iostream>
#include <fstream>
#include <mutex>
#include <vector>
#include <chrono>

namespace Pillar {

enum class LogLevel { Trace, Info, Warn, Error, Fatal };

struct LogEntry {
    LogLevel    level;
    std::string message;
    std::string timestamp;
};

class Log {
public:
    static void Init(const std::string& logFile = "pillar.log");
    static void Shutdown();

    template<typename... Args>
    static void Trace(std::format_string<Args...> fmt, Args&&... args) {
        Write(LogLevel::Trace, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void Info(std::format_string<Args...> fmt, Args&&... args) {
        Write(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void Warn(std::format_string<Args...> fmt, Args&&... args) {
        Write(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void Error(std::format_string<Args...> fmt, Args&&... args) {
        Write(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    static void Fatal(std::format_string<Args...> fmt, Args&&... args) {
        Write(LogLevel::Fatal, std::format(fmt, std::forward<Args>(args)...));
    }

    static const std::vector<LogEntry>& GetEntries() { return s_Entries; }
    static void Clear() { s_Entries.clear(); }

private:
    static void Write(LogLevel level, const std::string& msg);
    static std::string LevelToString(LogLevel level);
    static std::string GetTimestamp();

    static std::ofstream          s_File;
    static std::mutex             s_Mutex;
    static std::vector<LogEntry>  s_Entries;
};

} // namespace Pillar

#define PL_TRACE(...) ::Pillar::Log::Trace(__VA_ARGS__)
#define PL_INFO(...)  ::Pillar::Log::Info(__VA_ARGS__)
#define PL_WARN(...)  ::Pillar::Log::Warn(__VA_ARGS__)
#define PL_ERROR(...) ::Pillar::Log::Error(__VA_ARGS__)
#define PL_FATAL(...) ::Pillar::Log::Fatal(__VA_ARGS__)

#ifdef PILLAR_DEBUG
  #define PL_ASSERT(x, ...) { if(!(x)) { PL_FATAL("Assertion failed: {}", __VA_ARGS__); __debugbreak(); } }
#else
  #define PL_ASSERT(x, ...)
#endif
