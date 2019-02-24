#include "server.hpp"

Server::Server(WORD wPort) :
    m_wVersion{ MAKEWORD(2, 2) },
    m_WsaData{ 0 },
    m_ServSockInfo{ 0 },
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
        m_ServSockInfo.sin_family = AF_INET;
        m_ServSockInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
        m_ServSockInfo.sin_port = htons(wPort);
        bCont = (bind(m_hSocket, reinterpret_cast<sockaddr*>(&m_ServSockInfo), sizeof m_ServSockInfo) == 0);
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

        m_bStop = true;
        if (m_pAcceptThread != nullptr)
        {
            m_pAcceptThread->join();
        }
        if (m_pClientCommThread != nullptr)
        {
            m_pClientCommThread->join();
        }

        for (auto client : m_ClientsList)
        {
            if (client.first != INVALID_SOCKET)
            {
                closesocket(client.first);
            }
        }

        m_ClientsList.clear();
        m_ClientsMap.clear();
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

bool Server::RegisterClient(SOCKET hClientSocket, const sockaddr_in& clientSockAddr)
{
    std::lock_guard<std::mutex> guard(m_Mutex);
    if (m_bGood && !m_bStop && m_ClientsMap.find(hClientSocket) == m_ClientsMap.end())
    {
        m_ClientsList.push_back(std::make_pair(hClientSocket, clientSockAddr));
        m_ClientsMap[hClientSocket] = (m_ClientsList.size() - 1);

        return true;
    }

    return false;
}

bool Server::UnregisterClient(SOCKET hClientSocket)
{
    if (m_bGood && !m_bStop)
    {
        auto item = m_ClientsMap.find(hClientSocket);
        if (item != m_ClientsMap.end()) {
            m_ClientsList[item->second].first = INVALID_SOCKET;
            ::SecureZeroMemory(&m_ClientsList[item->second].second, sizeof(typename decltype(m_ClientsList)::value_type::second_type));
            m_ClientsMap.erase(item);
            
            return true;
        }
    }
    return false;
}

void Server::RunAcceptThread()
{
    if (m_bGood && m_pAcceptThread != nullptr) {
        m_pAcceptThread = std::make_unique<std::thread>([](Server* const self)
        {
            while (self != nullptr && *self && !self->IsServerStopped())
            {
                SOCKET hSocket = INVALID_SOCKET;
                sockaddr_in clientSocketAddr{ 0 };

                if (self->Accept(&hSocket, &clientSocketAddr))
                {
                    self->RegisterClient(hSocket, clientSocketAddr);
                }
            }
        }, this);
    }
}

void Server::RunClientCommThread()
{
    if (m_bGood && m_pAcceptThread != nullptr) {
        m_pClientCommThread = std::make_unique<std::thread>([](Server* const self)
        {
            while (self != nullptr && *self && !self->IsServerStopped())
            {
                std::vector<char> buffer;
                Message request, response;
                bool bBroadcast = false;
                SOCKET hConnectedHandle = INVALID_SOCKET;

                std::vector<fd_set> aSocketList;
                self->ReadySocketList(&aSocketList);

                if (self->ReadMessageFromClient(&buffer, aSocketList, &hConnectedHandle))
                {
                    if (self->AnalyzeMessage(buffer, &request))
                    {
                        self->ProcessRequestMessage(request, &response, &bBroadcast);
                    }
                    else
                    {
                        self->SetErrorNotificationMessage(&response, Message::AdminMessageType::kFailedMessageAnalysis);
                    }
                }
                else
                {
                    self->SetErrorNotificationMessage(&response, Message::AdminMessageType::kFailedMessageRead);
                }

                if (hConnectedHandle != INVALID_SOCKET)
                {
                    response.hConnectedSocket = hConnectedHandle;
                    self->WriteMessageToClient(response, bBroadcast);
                }
            }
        }, this);
    }
}

void Server::ReadySocketList(std::vector<fd_set>* pSocketList)
{
    if (m_bGood && pSocketList != nullptr)
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        fd_set sockets;
        sockets.fd_count = 0;
        for (auto& client : m_ClientsList)
        {
            if (client.first != INVALID_SOCKET)
            {
                if (sockets.fd_count >= FD_SETSIZE)
                {
                    pSocketList->push_back(sockets);
                    sockets.fd_count = 0;
                }
                sockets.fd_array[sockets.fd_count++] = client.first;
            }
        }

        if (sockets.fd_count > 0)
        {
            pSocketList->push_back(sockets);
        }
    }
}

bool Server::ReadMessageFromClient(std::vector<char>* aRawBuffer, std::vector<fd_set>& socketList, SOCKET* pConnectedSocket)
{
    if (m_bGood && aRawBuffer != nullptr && pConnectedSocket != nullptr)
    {
        *pConnectedSocket = INVALID_SOCKET;
        for (fd_set& sockets : socketList)
        {
            timeval tv{ kTimeIntervalForSelectBlocking , 0 };

            int iRet = select(0, &sockets, nullptr, nullptr, &tv);
            if (iRet != 0 && iRet != SOCKET_ERROR)
            {
                std::lock_guard<std::mutex> guard(m_Mutex);
                for (auto& client : m_ClientsList)
                {
                    if (client.first != INVALID_SOCKET && FD_ISSET(client.first, &sockets))
                    {
                        *pConnectedSocket = client.first;
                        
                        int iResult = 0;
                        char aBuffer[kInnerBufferSize]{ 0 };
                        do
                        {
                            iResult = recv(client.first, aBuffer, kInnerBufferSize, 0);
                            if (iResult == 0)
                            {
                                UnregisterClient(client.first);
                                *pConnectedSocket = INVALID_SOCKET;
                                break;
                            }
                            else if(iResult > 0)
                            {
                                size_t nOldSize = aRawBuffer->size();
                                aRawBuffer->resize(nOldSize + static_cast<size_t>(iResult));
                                CopyMemory(aRawBuffer->data() + nOldSize, aBuffer, iResult);
                            }
                        } while (iResult >= kInnerBufferSize);

                        break;
                    }
                }

                return (*pConnectedSocket != INVALID_SOCKET);
            }
        }
    }

    return false;
}

bool Server::AnalyzeMessage(const std::vector<char>& aRawBuffer, Message* pResultMessage)
{
    int64_t nBufferSize = aRawBuffer.size();
    if (pResultMessage != nullptr && nBufferSize > 0)
    {
        bool bCont = false;
        Message resultMessageTemp;

        const char* pBuffer = aRawBuffer.data();
        int64_t nHeaderSize = (sizeof(pResultMessage->uMessageType) + sizeof(pResultMessage->uMessageLength));
        bCont = (nBufferSize >= nHeaderSize);
        if (bCont)
        {
            resultMessageTemp.uMessageType = *reinterpret_cast<const uint32_t*>(pBuffer);
            resultMessageTemp.uMessageLength = *reinterpret_cast<const uint64_t*>(pBuffer + sizeof(pResultMessage->uMessageType));
        }

        int64_t nDataSize = (nBufferSize - nHeaderSize);
        bCont = bCont && (nDataSize == resultMessageTemp.uMessageLength);
        if (bCont)
        {
            pBuffer += nHeaderSize;
            resultMessageTemp.aRawMessage.resize(nDataSize);
            CopyMemory(resultMessageTemp.aRawMessage.data(), pBuffer, nDataSize);

            *pResultMessage = std::move(resultMessageTemp);
        }

        return bCont;
    }

    return false;
}

bool Server::WriteMessageToClient(const Message& writtenMessage, bool bBroadcast)
{
    if (bBroadcast || (writtenMessage.hConnectedSocket != INVALID_SOCKET))
    {
        int64_t nHeaderSize = sizeof(writtenMessage.uMessageType) + sizeof(writtenMessage.uMessageLength);
        std::unique_ptr<char> buffer(new char[nHeaderSize + writtenMessage.aRawMessage.size()]);
        if (buffer != nullptr)
        {
            *reinterpret_cast<uint32_t*>(buffer.get()) = static_cast<uint32_t>(writtenMessage.uMessageType);
            *reinterpret_cast<uint64_t*>(buffer.get() + sizeof(writtenMessage.uMessageType)) = static_cast<uint64_t>(writtenMessage.aRawMessage.size());
            CopyMemory(
                buffer.get() + sizeof(writtenMessage.uMessageType) + sizeof(writtenMessage.uMessageLength),
                writtenMessage.aRawMessage.data(),
                writtenMessage.aRawMessage.size());
        }

        if (buffer != nullptr)
        {
            std::vector<SOCKET> aRemovedSockets;

            if (bBroadcast)
            {
                for (auto& client : m_ClientsList)
                {
                    int iResult = send(client.first, buffer.get(), static_cast<int>(nHeaderSize + writtenMessage.aRawMessage.size()), 0);
                    if (iResult == 0 || iResult == SOCKET_ERROR)
                    {
                        aRemovedSockets.push_back(client.first);
                    }
                }
            }
            else
            {
                int iResult = send(writtenMessage.hConnectedSocket, buffer.get(), static_cast<int>(nHeaderSize + writtenMessage.aRawMessage.size()), 0);
                if (iResult == 0 || iResult == SOCKET_ERROR)
                {
                    aRemovedSockets.push_back(writtenMessage.hConnectedSocket);
                }
            }

            for (SOCKET hSocket : aRemovedSockets)
            {
                UnregisterClient(hSocket);
            }
        }

        return true;
    }

    return false;
}

void Server::SetErrorNotificationMessage(Message* pMessage, Message::AdminMessageType adminMessageType)
{
    if (pMessage != nullptr)
    {
        pMessage->uMessageType = static_cast<uint32_t>(Message::MessageType::kAdminMessage);
        pMessage->uMessageLength = 4;
        pMessage->aRawMessage.clear();
        pMessage->aRawMessage.resize(4);
        *(reinterpret_cast<uint32_t*>(pMessage->aRawMessage.data())) = static_cast<uint32_t>(adminMessageType);
    }
}

void Server::ProcessRequestMessage(const Message& requestMessage, Message* pResponseMessage, bool* pIsBroadcastMessage)
{

}