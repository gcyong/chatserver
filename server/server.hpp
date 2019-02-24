#ifndef CHATSERVER_SERVER_SERVER_HPP
#define CHATSERVER_SERVER_SERVER_HPP

#include <mutex>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <unordered_map>

#include <WinSock2.h>
#include <Windows.h>

#include "message.hpp"

class Server
{
    static constexpr int kMaximumWaitQueue = 100;
    static constexpr int kTimeIntervalForSelectBlocking = 5000;
    static constexpr int kInnerBufferSize = 128;
public :
    Server(WORD wPort = 8000);
    ~Server();

    WORD GetPort() const { return m_ServSockInfo.sin_port; }
    std::string GetBoundIPAddress() const { return std::string(inet_ntoa(m_ServSockInfo.sin_addr)); }

    bool Accept(SOCKET* pClientSocket, sockaddr_in* pClientSocketAddr);

    bool RegisterClient(SOCKET hClientSocket, const sockaddr_in& clientSockAddr);
    bool UnregisterClient(SOCKET hClientSocket);
    void RunAcceptThread();
    
    void RunClientCommThread();
    void ReadySocketList(std::vector<fd_set>* pSocketList);
    bool ReadMessageFromClient(std::vector<char>* aRawBuffer, std::vector<fd_set>& socketList, SOCKET* pConnectedSocket);
    bool AnalyzeMessage(const std::vector<char>& aRawBuffer, Message* pResultMessage);
    bool WriteMessageToClient(const Message& writtenMessage, bool bBroadcast = false);
    void SetErrorNotificationMessage(Message* pMessage, Message::AdminMessageType adminMessageType);
    void ProcessRequestMessage(const Message& requestMessage, Message* pResponseMessage, bool* pIsBroadcastMessage);

    inline void StopServer() { m_bStop = true; }
    inline bool IsServerStopped() const { return m_bStop; }
    explicit operator bool() { return m_bGood; }
private :
    WORD m_wVersion;
    WSADATA m_WsaData;
    SOCKET m_hSocket;
    sockaddr_in m_ServSockInfo;
    std::unordered_map<SOCKET, int64_t> m_ClientsMap;
    std::vector<std::pair<SOCKET, sockaddr_in>> m_ClientsList;

    bool m_bGood;
    bool m_bStop;

    std::mutex m_Mutex;
    std::unique_ptr<std::thread> m_pAcceptThread;
    std::unique_ptr<std::thread> m_pClientCommThread;
};

#endif
