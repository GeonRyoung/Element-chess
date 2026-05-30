#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <atomic>
#include <memory>
#include <array>
#include "CircularBuffer.h"

enum class SessionState : uint8_t
{
    Free = 0,       // LockFreeObjectPool 내부 대기
    Connecting,     // 연결 중
    WaitAuth,       // 인증 대기
    Active,         // 활성화
    Closed          // 종료
};

class RudpSession : public std::enable_shared_from_this<RudpSession>
{
private:
    SOCKET m_socket;
    uint64_t m_sessionId;
    
    std::atomic<SessionState> m_state;
    
    WSAOVERLAPPED m_recvOverlapped;
    WSAOVERLAPPED m_sendOverlapped;
    
    CircularBuffer m_recvBuffer;
    std::array<char, 2048> m_recvTempBuffer;
    std::atomic<bool> m_isRecvPending;
public:
    RudpSession();
    virtual ~RudpSession();
    
    void Init(SOCKET socket, uint64_t sessionId);
    
    void DisConnect();
    
    SessionState GetState() const {return m_state.load(std::memory_order_acquire);}
    bool SetState(SessionState expectedState, SessionState newState);
    
    void PostRecv();
    void PostSend();
    
    void OnRecvCompleted(size_t bytesTransferred);     
    
    /*=======================
            Getter
    =======================*/
    uint64_t GetSessionId() const { return m_sessionId; }
    WSAOVERLAPPED* GetRecvOverlappedPtr() { return &m_recvOverlapped; } 
    WSAOVERLAPPED* GetSendOverlappedPtr() { return &m_sendOverlapped; } 
    
};
