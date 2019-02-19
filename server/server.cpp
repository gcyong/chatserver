#include "server.hpp"

Server::Server(WORD wPort) :
    m_wVersion{ MAKEWORD(2, 2) },
    m_WsaData{ 0 },
    m_bGood{ false }
{
    bool bCont = false;
    if (WSAStartup(m_wVersion, &m_WsaData) == 0)
    {
        m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
        bCont = (m_hSocket != INVALID_SOCKET);
    }

    if (bCont)
    {
        sockaddr_in servSock{ 0 };
        servSock.sin_family = AF_INET;
        servSock.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
        servSock.sin_port = htons(wPort);
        bCont = (bind(m_hSocket, reinterpret_cast<sockaddr*>(&servSock), sizeof servSock) == 0);
    }

    if (bCont)
    {
        bCont = (listen(m_hSocket, kMaximumWaitQueue) == 0);
    }

    if (!bCont)
    {
        WSACleanup();
    }

    m_bGood = bCont;
}

Server::~Server()
{
    if (m_bGood)
    {
        WSACleanup();
        m_wVersion = 0;
        SecureZeroMemory(&m_WsaData, sizeof m_WsaData);
        m_bGood = false;
    }
}

bool Server::Accept(SOCKET* pClientSocket, sockaddr_in* pClientSocketAddr)
{
    if (m_bGood && pClientSocket != nullptr && pClientSocketAddr != nullptr)
    {
        int nClntSockAddrLen = sizeof *pClientSocketAddr;
        *pClientSocket = accept(m_hSocket, reinterpret_cast<sockaddr*>(pClientSocketAddr), &nClntSockAddrLen);

        return (*pClientSocket != INVALID_SOCKET);
    }

    return false;
}