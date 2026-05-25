#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>

class SessionManager;
class RudpSession;

class IocpCore
{
private:
    IocpCore(): m_hIocp(INVALID_HANDLE_VALUE), m_bIsRunning(false){}
    ~IocpCore() { Shutdown();}
    
    IocpCore(const IocpCore&) = delete;
    IocpCore& operator=(const IocpCore&) = delete;
    
    HANDLE m_hIocp;
    std::vector<std::thread> m_workerThreads;
    std::shared_ptr<SessionManager> m_sessionManager;
    std::atomic<bool> m_bIsRunning;
    
    void WorkerThreadMain();
public:
    static IocpCore& GetInstance()
    {
        static IocpCore instance;
        return instance;
    }
    
    bool CreateIocpPort(DWORD numThreads = 0);
    bool RegisterSocket(SOCKET socket, ULONG_PTR completionKey);
    
    bool IsRunning() const
    {
        return m_bIsRunning.load(std::memory_order_relaxed);
    }
    
    void Shutdown();
    
    void SetSessionMnager(std::shared_ptr<SessionManager> manager)
    {
        m_sessionManager = manager;
    }
};
