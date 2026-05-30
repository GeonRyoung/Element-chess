#include "RdupSession.h"
#include <iostream>

#include "Packet.h"

RudpSession::RudpSession(): m_socket(INVALID_SOCKET), m_sessionId(0), m_state(SessionState::Free), 
                            m_recvBuffer(8192), m_isRecvPending(false)
{
    ZeroMemory(&m_recvOverlapped, sizeof(m_recvOverlapped));
    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));
    m_recvTempBuffer.fill(0);
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
    
    m_recvBuffer.Clear();
    m_isRecvPending.store(false, std::memory_order_release);
    
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
    
    m_isRecvPending.store(false, std::memory_order_release);
    m_recvBuffer.Clear();
    
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
    
    bool expected = false;
    if (!m_isRecvPending.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;

    ZeroMemory(&m_recvOverlapped, sizeof(m_recvOverlapped));

    WSABUF wsaBuf;
    wsaBuf.buf = m_recvTempBuffer.data(); 
    wsaBuf.len = static_cast<ULONG>(m_recvTempBuffer.size());

    DWORD bytesReceived = 0;
    DWORD flags = 0;

    if (WSARecv(m_socket, &wsaBuf, 1, &bytesReceived, &flags, &m_recvOverlapped, nullptr) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            m_isRecvPending.store(false, std::memory_order_release); 
            DisConnect();
        }
    }
}

void RudpSession::PostSend()
{
}

void RudpSession::OnRecvCompleted(size_t bytesTransferred)
{
    m_isRecvPending.store(false, std::memory_order_release);

    if (GetState() == SessionState::Closed || GetState() == SessionState::Free)
        return;

    if (bytesTransferred == 0)
    {
        DisConnect();
        return;
    }

    if (!m_recvBuffer.Write(m_recvTempBuffer.data(), bytesTransferred))
    {
        DisConnect();
        return;
    }

    while (true)
    {
        PacketHeader header{};
        if (!m_recvBuffer.Peek(reinterpret_cast<char*>(&header), sizeof(PacketHeader)))
            break; 

        if (header.size < sizeof(PacketHeader) || header.size > MAX_PACKET_SIZE)
        {
            DisConnect();
            return;
        }

        if (m_recvBuffer.GetUseSize() < header.size)
            break; 

        std::array<char, MAX_PACKET_SIZE> onePacket{};
        if (!m_recvBuffer.Peek(onePacket.data(), header.size))
        {
            DisConnect();
            return;
        }

        if (!m_recvBuffer.Consume(header.size))
        {
            DisConnect();
            return;
        }

        // 3) TODO: PacketHandler로 디스패치
        const PacketHeader* parsedHeader = reinterpret_cast<const PacketHeader*>(onePacket.data());
        (void)parsedHeader;
    }

    PostRecv();
}