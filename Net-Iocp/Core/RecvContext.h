#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdint>

constexpr size_t MAX_RECV_BUFFER_SIZE = 8192;

enum class IocpOpType
{
    Recv,
    Send
};

struct IocpContext
{
    WSAOVERLAPPED overlapped; // 무조건 구조체 가장 처음에 위치해야 함!
    IocpOpType type;
    uint64_t sessionId = 0;
};

struct RecvContext : public IocpContext
{
    WSABUF wsaBuf;
    char buffer[MAX_RECV_BUFFER_SIZE];
    SOCKADDR_IN remoteAddr;
    int remoteAddrLen;

    void Init()
    {
        ZeroMemory(&overlapped, sizeof(overlapped));
        type = IocpOpType::Recv;
        wsaBuf.buf = buffer;
        wsaBuf.len = MAX_RECV_BUFFER_SIZE;
        ZeroMemory(&remoteAddr, sizeof(remoteAddr));
        remoteAddrLen = sizeof(SOCKADDR_IN);
    }
};
