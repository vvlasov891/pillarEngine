#include "Log.h"

namespace Pillar {

std::ofstream         Log::s_File;
std::mutex            Log::s_Mutex;
std::vector<LogEntry> Log::s_Entries;

void Log::Init(const std::string& logFile) {
    s_File.open(logFile, std::ios::out | std::ios::trunc);
    Info("PillarEngine logger initialized.");
}

void Log::Shutdown() {
    if (s_File.is_open()) s_File.close();
}

void Log::Write(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(s_Mutex);
    std::string ts  = GetTimestamp();
    std::string lvl = LevelToString(level);
    std::string line = std::format("[{}] [{}] {}", ts, lvl, msg);

    std::cout << line << "\n";
    if (s_File.is_open()) { s_File << line << "\n"; s_File.flush(); }

    s_Entries.push_back({ level, msg, ts });
    if (s_Entries.size() > 4096) s_Entries.erase(s_Entries.begin());
}

std::string Log::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "?????";
}

std::string Log::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return buf;
}

} // namespace Pillar
