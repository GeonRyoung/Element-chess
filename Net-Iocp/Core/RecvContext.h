#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdint>

constexpr size_t MAX_RECV_BUFFER_SIZE = 8192;

struct RecvContext
{
    WSAOVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[MAX_RECV_BUFFER_SIZE];
    SOCKADDR_IN remoteAddr;
    int remoteAddrLen;

    void Init()
    {
        ZeroMemory(&overlapped, sizeof(overlapped));
        wsaBuf.buf = buffer;
        wsaBuf.len = MAX_RECV_BUFFER_SIZE;
        ZeroMemory(&remoteAddr, sizeof(remoteAddr));
        remoteAddrLen = sizeof(SOCKADDR_IN);
    }
};
