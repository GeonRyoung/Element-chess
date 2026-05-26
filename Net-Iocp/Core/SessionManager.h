#pragma once

#include "RdupSession.h"
#include "LockFreeObjectPool.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <atomic>

#include "IocpCore.h"

class SessionManger
{
private:
    SessionManger() = default;
    ~SessionManger() = default;
    SessionManger(const SessionManager&) = delete;
    SessionManger& operator=(const SessionManager&) = delete;
    
    LockFreeObjectPool<RudpSession> m_sessionPool;
    
    mutable std::shared_mutex m_sessionMapLock;
    std::unordered_map<uint64_t, std::shared_ptr<RudpSession>> m_activeSessions;
    
    std::atomic<uint64_t> m_sessionIdGenerator{1 };
    
public:
    static SessionManger& GetInstance()
    {
        static SessionManger instance;
        return instance;
    }
    
    std::shared_ptr<RudpSession> CreateSession(SOCKET socket)
    {
        std::shared_ptr<RudpSession> session = m_sessionPool.Alloc();
        if (!session)
            return nullptr;
        
        uint64_t newId = m_sessionIdGenerator.fetch_add(1, std::memory_order_acquire); 
        session->SetSessionId(newId);
        
        {
            std::unique_lock<std::shared_mutex> lock(m_sessionMapLock);
            m_activeSessions[newId] = session;
        }
        
        return session;
    }
    
    void RemoveSession(uint64_t sessionId)
    {
        std::unique_lock<std::shared_mutex> lock(m_sessionMapLock);
        m_activeSessions.erase(sessionId);
    }
    
    std::shared_ptr<RudpSession> FindSession(uint64_t sessionId) const
    {
        std::shared_lock<std::shared_mutex> lock(m_sessionMapLock);
        
        auto it = m_activeSessions.find(sessionId);
        if (it == m_activeSessions.end())
        {
            return it->second;
        }
        return nullptr;
    }
};
