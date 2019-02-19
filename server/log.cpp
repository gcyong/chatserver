#include "log.hpp"

std::map<Log::Level, std::string> Log::s_LevelToStr;

void Log::InitLevelToStringMapper()
{
    s_LevelToStr[Level::kAll] = "ALL";
    s_LevelToStr[Level::kDebug] = "DEBUG";
    s_LevelToStr[Level::kError] = "ERROR";
    s_LevelToStr[Level::kFatal] = "FATAL";
    s_LevelToStr[Level::kInfo] = "INFO";
    s_LevelToStr[Level::kWarn] = "WARN";
}

Log::Log(std::ostream& out) :
    m_Out(out),
    m_LogLevel(Level::kAll)
{
}

Log& Log::operator<<(Log::Level logLevel)
{
    m_LogLevel = logLevel;

    return *this;
}

Log& Log::operator<<(const std::string& strMessage)
{
    std::chrono::system_clock::time_point&& nowtp = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(nowtp);

    std::string nowStr(ctime(&now));
    if (nowStr.back() == '\n') {
        nowStr.pop_back();
    }

    m_Out << "[" << s_LevelToStr[m_LogLevel] << "]" << "[" << nowStr << "] " << strMessage.c_str() << std::endl;

    return *this;
}