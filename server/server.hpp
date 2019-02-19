#ifndef CHATSERVER_SERVER_SERVER_HPP
#define CHATSERVER_SERVER_SERVER_HPP

#include <string>

#include <WinSock2.h>
#include <Windows.h>

class Server
{
    static constexpr int kMaximumWaitQueue = 100;
public :
    Server(WORD wPort = 8000);
    ~Server();

    bool Accept(SOCKET* pClientSocket, sockaddr_in* pClientSocketAddr);

    explicit operator bool() { return m_bGood; }
private :
    WORD m_wVersion;
    WSADATA m_WsaData;
    SOCKET m_hSocket;

    bool m_bGood;
};

#endif
