#ifndef CHATSERVER_SERVER_MESSAGE_H
#define CHATSERVER_SERVER_MESSAGE_H

#include <vector>
#include <cstdint>

struct Message
{
public :
    enum class MessageType
    {
        kAdminMessage,
        kGeneralMessage,
        kSecretMessage,
        kFileMessage,
    };

    enum class AdminMessageType
    {
        kRequestOk,
        kFailedMessageRead,
        kFailedMessageAnalysis,
        kRegisterNickname,
        kUnknownError,
        kInvalidNicknameLength,
        kInvalidNicknamePacket,
        kInvalidGeneralMessageLength,
        kInvalidGeneralMessagePacket,
        kUnregisteredUser,
        kNicknameEmpty,
        kNicknameAlreadyExist,
    };

    std::uint32_t uMessageType;
    std::uint64_t uMessageLength;
    std::vector<char> aRawMessage;

    SOCKET hConnectedSocket;
};

#endif