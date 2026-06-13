#pragma once

#include "RudpSession.h"
#include "LockFreeObjectPool.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <atomic>

#include "IocpCore.h"
#include "PacketDispatcher.h"
#include "RecvContext.h"

class SessionManager
{
private:
    SessionManager() = default;
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    LockFreeObjectPool<RudpSession> m_sessionPool;
    LockFreeObjectPool<Packet> m_packetPool;
    
    mutable std::shared_mutex m_sessionMapLock;
    std::unordered_map<uint64_t, std::shared_ptr<RudpSession>> m_activeSessions;
    std::unordered_map<uint64_t, std::shared_ptr<RudpSession>> m_sessionByAddr;
    
    std::atomic<uint64_t> m_sessionIdGenerator{1 };
    SOCKET m_serverSocket = INVALID_SOCKET;
    
    class TimerWheel* m_pTimerWheel = nullptr;
    PacketDispatcher m_packetDispatcher;
    
public:
    static SessionManager& GetInstance()
    {
        static SessionManager instance;
        return instance;
    }
    
    void SetServerSocket(SOCKET sock) { m_serverSocket = sock; }
    
    uint64_t GetAddrKey(const SOCKADDR_IN& addr) const
    {
        return (static_cast<uint64_t>(addr.sin_addr.s_addr) << 32) | addr.sin_port;
    }

    void OnReceiveRouted(RecvContext* context, DWORD bytesReceived)
    {
        if (bytesReceived == 0) return;
        
        uint64_t addrKey = GetAddrKey(context->remoteAddr);
        std::shared_ptr<RudpSession> session;
        {
            std::shared_lock<std::shared_mutex> lock(m_sessionMapLock);
            auto it = m_sessionByAddr.find(addrKey);
            if (it != m_sessionByAddr.end())
            {
                session = it->second;
            }
        }
        
        if (!session)
        {
            session = m_sessionPool.Alloc();
            if (!session) return;
            
            uint64_t newId = m_sessionIdGenerator.fetch_add(1, std::memory_order_acquire); 
            session->Init(m_serverSocket, newId, context->remoteAddr);
            
            {
                std::unique_lock<std::shared_mutex> lock(m_sessionMapLock);
                m_activeSessions[newId] = session;
                m_sessionByAddr[addrKey] = session;
            }
        }
        
        session->OnRecvData(context->buffer, bytesReceived);
    }
    
    void PostRecvFrom(RecvContext* context)
    {
        if (m_serverSocket == INVALID_SOCKET) return;
        
        DWORD bytesRecv = 0;
        DWORD flags = 0;
        context->remoteAddrLen = sizeof(SOCKADDR_IN);
        ZeroMemory(&context->overlapped, sizeof(WSAOVERLAPPED));
        
        WSARecvFrom(m_serverSocket, &context->wsaBuf, 1, &bytesRecv, &flags, 
                   (SOCKADDR*)&context->remoteAddr, &context->remoteAddrLen, 
                   &context->overlapped, nullptr);
    }
    
    std::shared_ptr<RudpSession> CreateSession(SOCKET socket)
    {
        // 원본 CreateSession은 하위 호환성을 위해 남겨두거나 삭제 가능하나 안전을 위해 수정
        return nullptr;
    }
    
    void RemoveSession(uint64_t sessionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionMapLock);
        auto it = m_activeSessions.find(sessionId);
        if (it != m_activeSessions.end())
        {
            uint64_t addrKey = GetAddrKey(it->second->GetRemoteAddr());
            m_sessionByAddr.erase(addrKey);
            m_activeSessions.erase(it);
        }
    }
    
    std::shared_ptr<RudpSession> FindSession(uint64_t sessionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionMapLock);
        auto it = m_activeSessions.find(sessionId);
        if (it == m_activeSessions.end())
            return nullptr;
        return it->second;
    }

    size_t GetActiveSessionCount() const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionMapLock);
        return m_activeSessions.size();
    }

    size_t GetSessionPoolAvailableCount() const
    {
        return m_sessionPool.GetAvailableCount();
    }
    
    std::shared_ptr<Packet> AllocPacket()
    {
        return m_packetPool.Alloc();
    }
    
    void SetTimerWheel(class TimerWheel* tw) { m_pTimerWheel = tw; }
    class TimerWheel* GetTimerWheel() const { return m_pTimerWheel; }
    
    PacketDispatcher* GetPacketDispatcher() { return &m_packetDispatcher; }
};
