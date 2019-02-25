#include <type_traits>
#include <iostream>

#include "log.hpp"
#include "server.hpp"
#include "client.hpp"

int main(int argc, char* argv[])
{
    // 로그 초기화
    Log::InitLevelToStringMapper();
    Log log(std::cout);
    log << Log::Level::kInfo << "Start chatserver...";


    // 서버 준비
    log << Log::Level::kInfo << "Ready chatserver...";
    Server server(8000);

    if (server)
    {
        log << Log::Level::kInfo
            << "Successful initialization chatserver..."
            << ("\t\tIP = " + server.GetBoundIPAddress()).c_str()
            << ("\t\tPORT = " + std::to_string(server.GetPort())).c_str();

        //server.RunAcceptThread();
        //server.RunClientCommThread();
    }
    else
    {
        log << Log::Level::kError << "Failed to initialize chatserver.";
    }

    log << Log::Level::kInfo << "End chatserver...";
}