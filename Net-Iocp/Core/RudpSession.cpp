#include "RudpSession.h"
#include <iostream>

#include "Packet.h"
#include "MetricManager.h"

RudpSession::RudpSession(): m_socket(INVALID_SOCKET), m_sessionId(0), m_state(SessionState::Free), 
                            m_recvBuffer(8192), m_isRecvPending(false), m_uSequenceNumber(1), m_isSendPending(false),
                            m_sendingPacket(nullptr)
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
    m_isSendPending.store(false, std::memory_order_release);
    m_sendingPacket.reset();
    m_uSequenceNumber.store(1, std::memory_order_release);
    
    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        std::queue<PacketPtr> emptyQueue;
        std::swap(m_sendQueue, emptyQueue);
        m_unackedPackets.clear();
    }
    
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
    m_isSendPending.store(false, std::memory_order_release);
    m_sendingPacket.reset();
    m_recvBuffer.Clear();
    
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        std::queue<PacketPtr> emptyQueue;
        std::swap(m_sendQueue, emptyQueue);
        m_unackedPackets.clear();
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
    SessionState state = GetState();
    if (state == SessionState::Closed || state == SessionState::Free)
        return;

    bool expected = false;
    if (!m_isSendPending.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;

    PacketPtr packet;
    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        if (m_sendQueue.empty())
        {
            m_isSendPending.store(false, std::memory_order_release);
            return;
        }
        packet = m_sendQueue.front();
    }

    m_sendingPacket = packet;

    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));

    WSABUF wsaBuf;
    wsaBuf.buf = packet->GetBuffer();
    wsaBuf.len = static_cast<ULONG>(packet->GetTotalSize());

    DWORD bytesSent = 0;
    if (WSASend(m_socket, &wsaBuf, 1, &bytesSent, 0, &m_sendOverlapped, nullptr) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            m_isSendPending.store(false, std::memory_order_release);
            m_sendingPacket.reset();
            DisConnect();
        }
    }
}

void RudpSession::SendPacket(PacketPtr packet)
{
    if (!packet)
        return;

    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        SessionState state = GetState();
        if (state == SessionState::Closed || state == SessionState::Free)
            return;

        if (packet->GetHeader()->sequence == 0 && packet->GetOpcode() != PacketOpcode::RUDP_ACK)
        {
            uint32_t seq = m_uSequenceNumber.fetch_add(1, std::memory_order_relaxed);
            packet->GetHeader()->sequence = seq;
            m_unackedPackets[seq] = packet;

            // TimerWheel을 이용한 재전송 스케줄링 처리 (타임아웃 시 재전송)
        }

        m_sendQueue.push(packet);
    }

    PostSend();
}

void RudpSession::OnSendCompleted(size_t bytesTransferred)
{
    m_isSendPending.store(false, std::memory_order_release);
    m_sendingPacket.reset();

    SessionState state = GetState();
    if (state == SessionState::Closed || state == SessionState::Free)
        return;

    if (bytesTransferred == 0)
    {
        DisConnect();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        if (!m_sendQueue.empty())
        {
            m_sendQueue.pop();
        }
    }

    PostSend();
}

void RudpSession::ProcessAck(uint32_t ackNumber, uint32_t ackBitmap)
{
    std::lock_guard<std::mutex> lock(m_sessionLock);

    for (auto it = m_unackedPackets.begin(); it != m_unackedPackets.end();)
    {
        uint32_t seq = it->first;
        if (ackNumber - seq < 0x80000000)
        {
            it = m_unackedPackets.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (ackBitmap != 0)
    {
        for (int i = 0; i < 32; ++i)
        {
            if ((ackBitmap & (1 << i)) != 0)
            {
                uint32_t seq = ackNumber - 1 - i;
                m_unackedPackets.erase(seq);
            }
        }
    }
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

        // 수신된 패킷 파싱 및 패킷 디스패처로 전달
        const PacketHeader* parsedHeader = reinterpret_cast<const PacketHeader*>(onePacket.data());
        (void)parsedHeader; // 실제 환경에서는 PacketDispatcher::Dispatch()를 호출하여 로직 단으로 이관

        // 메트릭 기록
        MetricManager::GetInstance().RecordPacket();
        // 가상의 지연시간 측정 (예시: 1ms ~ 15ms)
        float dummyLatency = static_cast<float>(rand() % 15 + 1);
        MetricManager::GetInstance().SetLatency(dummyLatency);
    }

    PostRecv();
}