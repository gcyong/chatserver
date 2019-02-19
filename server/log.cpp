#include "log.hpp"

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

    m_Out << "["
}