#ifndef CHATSERVER_SERVER_CLIENT_HPP
#define CHATSERVER_SERVER_CLIENT_HPP

#include <WinSock2.h>
#include <Windows.h>

template <typename SockAddrType>
class Client
{
public :
    Client(SOCKET hSocket, SockAddrType sockAddress) :
        m_hSocket(hSocket),
        m_SockAddress(sockAddress)
    {
    }

    ~Client();

private :
    SOCKET m_hSocket;
    SockAddrType m_SockAddress;
};

#endif
