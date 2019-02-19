#ifndef CHATSERVER_SERVER_LOG_HPP
#define CHATSERVER_SERVER_LOG_HPP

#include <string>
#include <ostream>
#include <chrono>

class Log
{
public :
    enum class Level
    {
        kAll,
        kDebug,
        kInfo,
        kWarn,
        kError,
        kFatal,
    };

public :
    Log(std::ostream& out);

    Log& operator<<(Level logLevel);
    Log& operator<<(const std::string& strMessage);

private :
    Level m_LogLevel;
    std::ostream& m_Out;
};

#endif
