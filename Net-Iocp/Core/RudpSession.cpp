#include "RudpSession.h"

#include "Packet.h"
#include "MetricManager.h"
#include "SessionManager.h"
#include "TimerWheel.h"
#include <algorithm>
#include <cmath>

RudpSession::RudpSession(): m_socket(INVALID_SOCKET), m_sessionId(0), m_state(SessionState::Free), 
                            m_recvBuffer(8192), m_isRecvPending(false), m_uSequenceNumber(1), m_isSendPending(false),
                            m_sendingPacket(nullptr), m_fSrtt(0.0f), m_fRttVar(0.0f), m_uRto(500),
                            m_highestRecvSeq(0), m_recvBitmap(0)
{
    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));
    ZeroMemory(&m_remoteAddr, sizeof(m_remoteAddr));
}

RudpSession::~RudpSession()
{
    DisConnect();
}

void RudpSession::Init(SOCKET socket, uint64_t sessionId, const SOCKADDR_IN& remoteAddr)
{
    m_socket = socket;
    m_sessionId = sessionId;
    m_remoteAddr = remoteAddr;
    
    m_state.store(SessionState::Connecting, std::memory_order_release);
    
    ZeroMemory(&m_sendOverlapped, sizeof(m_sendOverlapped));
    
    m_recvBuffer.Clear();
    m_isSendPending.store(false, std::memory_order_release);
    m_sendingPacket.reset();
    m_uSequenceNumber.store(1, std::memory_order_release);
    m_fSrtt.store(0.0f, std::memory_order_release);
    m_fRttVar.store(0.0f, std::memory_order_release);
    m_uRto.store(500, std::memory_order_release);
    m_highestRecvSeq.store(0, std::memory_order_release);
    m_recvBitmap.store(0, std::memory_order_release);
    
    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        std::queue<PacketPtr> emptyQueue;
        std::swap(m_sendQueue, emptyQueue);
        m_unackedPackets.clear();
    }
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
    
    m_isSendPending.store(false, std::memory_order_release);
    m_sendingPacket.reset();
    m_recvBuffer.Clear();

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
    // WSASendTo 사용 (클라이언트의 정확한 주소로 응답)
    if (WSASendTo(m_socket, &wsaBuf, 1, &bytesSent, 0, (SOCKADDR*)&m_remoteAddr, sizeof(m_remoteAddr), &m_sendOverlapped, nullptr) == SOCKET_ERROR)
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
            
            UnackedInfo info;
            info.packet = packet;
            info.sendTime = std::chrono::steady_clock::now();
            info.retransmitCount = 0;
            m_unackedPackets[seq] = info;

            // TimerWheel을 이용한 재전송 스케줄링 처리 (타임아웃 시 재전송)
            uint32_t delayTicks = (std::max)(1u, m_uRto.load(std::memory_order_relaxed) / 16);
            auto self = shared_from_this();
            if (SessionManager::GetInstance().GetTimerWheel()) {
                SessionManager::GetInstance().GetTimerWheel()->AddTimer(delayTicks, [self, seq]() {
                    self->ResendPacket(seq);
                });
            }
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
            if (seq == ackNumber) {
                auto now = std::chrono::steady_clock::now();
                float sampleRtt = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.sendTime).count());
                
                float srtt = m_fSrtt.load(std::memory_order_relaxed);
                float rttVar = m_fRttVar.load(std::memory_order_relaxed);
                
                if (srtt == 0.0f) {
                    srtt = sampleRtt;
                    rttVar = sampleRtt / 2.0f;
                } else {
                    const float alpha = 0.125f;
                    const float beta = 0.25f;
                    rttVar = (1.0f - beta) * rttVar + beta * std::abs(srtt - sampleRtt);
                    srtt = (1.0f - alpha) * srtt + alpha * sampleRtt;
                }
                
                m_fSrtt.store(srtt, std::memory_order_relaxed);
                m_fRttVar.store(rttVar, std::memory_order_relaxed);
                
                uint32_t newRto = static_cast<uint32_t>(srtt + (std::max)(10.0f, 4.0f * rttVar));
                newRto = (std::max)(100u, (std::min)(newRto, 5000u)); // Clamp [100ms, 5000ms]
                m_uRto.store(newRto, std::memory_order_relaxed);
            }
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

void RudpSession::ResendPacket(uint32_t sequence)
{
    {
        std::lock_guard<std::mutex> lock(m_sessionLock);
        auto it = m_unackedPackets.find(sequence);
        if (it == m_unackedPackets.end())
            return; // 이미 ACK 됨
            
        SessionState state = GetState();
        if (state == SessionState::Closed || state == SessionState::Free)
            return;

        // 재전송 로직
        it->second.retransmitCount++;
        MetricManager::GetInstance().RecordRetransmit();
        
        // 지수 백오프
        uint32_t currentRto = m_uRto.load(std::memory_order_relaxed);
        currentRto = (std::min)(currentRto * 2, 5000u);
        m_uRto.store(currentRto, std::memory_order_relaxed);
        
        // 다시 SendQueue에 넣기
        m_sendQueue.push(it->second.packet);
        
        // 다음 타이머 등록
        uint32_t delayTicks = (std::max)(1u, currentRto / 16);
        auto self = shared_from_this();
        if (SessionManager::GetInstance().GetTimerWheel()) {
            SessionManager::GetInstance().GetTimerWheel()->AddTimer(delayTicks, [self, sequence]() {
                self->ResendPacket(sequence);
            });
        }
    }
    
    PostSend();
}

void RudpSession::OnRecvData(char* data, size_t bytesTransferred)
{
    if (GetState() == SessionState::Closed || GetState() == SessionState::Free)
        return;

    if (bytesTransferred == 0)
    {
        DisConnect();
        return;
    }

    if (!m_recvBuffer.Write(data, bytesTransferred))
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

        // 수신된 패킷 파싱 및 패킷 디스패처로 전달 (Object Pool 적용)
        PacketPtr receivedPacket = SessionManager::GetInstance().AllocPacket();
        std::memcpy(receivedPacket->GetBuffer(), onePacket.data(), header.size);
        
        // RUDP 수신 시퀀스 갱신 및 비트맵 기록
        uint32_t seq = header.sequence;
        if (seq > 0 && header.opcode != static_cast<uint16_t>(PacketOpcode::RUDP_ACK))
        {
            uint32_t highest = m_highestRecvSeq.load(std::memory_order_relaxed);
            uint32_t bitmap = m_recvBitmap.load(std::memory_order_relaxed);
            
            if (seq > highest)
            {
                uint32_t diff = seq - highest;
                if (diff >= 32)
                {
                    bitmap = 0;
                }
                else
                {
                    bitmap = (bitmap << diff) | (1 << (diff - 1));
                }
                m_highestRecvSeq.store(seq, std::memory_order_relaxed);
                m_recvBitmap.store(bitmap, std::memory_order_relaxed);
                highest = seq;
            }
            else if (highest - seq <= 32 && seq < highest)
            {
                uint32_t diff = highest - seq;
                bitmap |= (1 << (diff - 1));
                m_recvBitmap.store(bitmap, std::memory_order_relaxed);
            }
            
            // RUDP_ACK 송신 (풀을 통해 할당)
            PacketPtr ackPacket = SessionManager::GetInstance().AllocPacket();
            ackPacket->Init(PacketOpcode::RUDP_ACK, 0);
            RudpAckBody ackBody;
            ackBody.ackNumber = highest;
            ackBody.ackBitmap = bitmap;
            ackPacket->Write(ackBody);
            
            SendPacket(ackPacket);
        }
        
        SessionManager::GetInstance().GetPacketDispatcher()->Dispatch(shared_from_this(), receivedPacket);

        // 메트릭 기록
        MetricManager::GetInstance().RecordPacket();
        // 가상의 지연시간 측정 (예시: 1ms ~ 15ms)
        float dummyLatency = static_cast<float>(rand() % 15 + 1);
        MetricManager::GetInstance().SetLatency(dummyLatency);
    }
}