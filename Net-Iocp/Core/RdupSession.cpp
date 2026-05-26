#include "RdupSession.h"
#include <iostream>

RudpSession::RudpSession(): m_socket(INVALID_SOCKET), m_sessionId(0), m_state(SessionState::Free)
{
    ZeroMemory(&m_recvOverlapped, sizeof(m_recvOverlapped));
    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));
}

RudpSession::~RudpSession()
{
    DisConnect();
}

void RudpSession::Init(SOCKET socket, uint64_t sessionId)
{
    m_socket = socket;
    m_sessionId = sessionId;
    
    m_state.store(SessionState::Connecting, std::memory_order_release);
    
    ZeroMemory(&m_recvOverlapped, sizeof(m_recvOverlapped));
    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));
    
    PostRecv();
}

void RudpSession::DisConnect()
{
    SessionState expected = m_state.load(std::memory_order_acquire);

    do
    {
        if (expected == SessionState::Closed || expected == SessionState::Free)
            return;
    }
    while (!m_state.compare_exchange_weak(expected, SessionState::Closed,
        std::memory_order_acq_rel, std::memory_order_acquire));
    
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool RudpSession::SetState(SessionState expectedState, SessionState newState)
{
    return m_state.compare_exchange_strong(
        expectedState, 
        newState, 
        std::memory_order_acq_rel, 
        std::memory_order_acquire
    );
}

void RudpSession::PostRecv()
{
    if (GetState() == SessionState::Closed || GetState() == SessionState::Free)
        return;
    
    WSABUF wsaBuf;
    
    //[TODO] 실제 CiruclaBuffer와 연결해야함. 임시 더미 테스트
    char dummyBuf[1024]; 
    wsaBuf.buf = dummyBuf;
    wsaBuf.len = sizeof(dummyBuf);

    DWORD bytesReceived = 0;
    DWORD flags = 0;
    
    if (WSARecv(m_socket, &wsaBuf, 1, &bytesReceived, &flags, &m_recvOverlapped, nullptr) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
            DisConnect();
    }
}

void RudpSession::PostSend()
{
}
