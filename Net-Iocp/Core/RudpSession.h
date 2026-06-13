#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <atomic>
#include <memory>
#include <array>
#include <queue>
#include <unordered_map>
#include <mutex>
#include "CircularBuffer.h"
#include "Packet.h"
#include "RecvContext.h"
#include <chrono>

struct UnackedInfo
{
    PacketPtr packet;
    std::chrono::steady_clock::time_point sendTime;
    int retransmitCount;
};

enum class SessionState : uint8_t
{
    Free = 0,
    Connecting,
    WaitAuth,
    Active,
    Closed
};

class RudpSession : public std::enable_shared_from_this<RudpSession>
{
private:
    SOCKET m_socket;
    uint64_t m_sessionId;
    
    std::atomic<SessionState> m_state;
    SOCKADDR_IN m_remoteAddr;
    
    IocpContext m_sendContext;
    
    CircularBuffer m_recvBuffer;
    std::atomic<bool> m_isRecvPending;
    
    std::queue<PacketPtr> m_sendQueue;
    std::unordered_map<uint32_t, UnackedInfo> m_unackedPackets;
    std::mutex m_sessionLock;
    std::atomic<uint32_t> m_uSequenceNumber;
    std::atomic<bool> m_isSendPending;
    PacketPtr m_sendingPacket;

    // RTT & RTO (Congestion Control)
    std::atomic<float> m_fSrtt;
    std::atomic<float> m_fRttVar;
    std::atomic<uint32_t> m_uRto;
    
    // RUDP Receive Tracking
    std::atomic<uint32_t> m_highestRecvSeq;
    std::atomic<uint32_t> m_recvBitmap;

public:
    RudpSession();
    virtual ~RudpSession();
    
    void Init(SOCKET socket, uint64_t sessionId, const SOCKADDR_IN& remoteAddr);
    
    void DisConnect();
    
    SessionState GetState() const {return m_state.load(std::memory_order_acquire);}
    bool SetState(SessionState expectedState, SessionState newState);
    
    void PostSend();
    void SendPacket(PacketPtr packet);
    
    void OnRecvData(char* data, size_t bytesTransferred);     
    void OnSendCompleted(size_t bytesTransferred);
    void ProcessAck(uint32_t ackNumber, uint32_t ackBitmap = 0);
    
    void ResendPacket(uint32_t sequence);
    
    /*=======================
            Getter
    =======================*/
    uint64_t GetSessionId() const { return m_sessionId; }
    IocpContext* GetSendContextPtr() { return &m_sendContext; } 
    const SOCKADDR_IN& GetRemoteAddr() const { return m_remoteAddr; }
    
};
