#include "IocpCore.h"
#include <iostream>

void IocpCore::WorkerThreadMain()
{
    DWORD bytesTransferred = 0;
    ULONG_PTR completionKey = 0;
    LPOVERLAPPED pOverlapped = nullptr;
    
    while (IsRunning())
    {
        BOOL bSuccess = GetQueuedCompletionStatus(
            m_hIocp,
            &bytesTransferred,
            &completionKey,
            &pOverlapped,
            INFINITE
            );
        
        if (m_bIsRunning.load(std::memory_order_acquire) == false)
            break;
        
        if (bSuccess && pOverlapped != nullptr)
        {
            // [TODO]
        }
        else
        {
            // [Todo] 
        }
    }
}

bool IocpCore::CreateIocpPort(DWORD numThreads)
{
    m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numThreads);
    if (m_hIocp == NULL)
        return false;
    
    m_bIsRunning.store(true, std::memory_order_release);
    
    if (numThreads == 0)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        numThreads = sysInfo.dwNumberOfProcessors * 2;
    }
    
    for (DWORD i = 0; i < numThreads; i++)
        m_workerThreads.emplace_back([this](){WorkerThreadMain();});
    
    return true;
}

bool IocpCore::RegisterSocket(SOCKET socket, ULONG_PTR completionKey)
{
    HANDLE hResult = CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), m_hIocp, completionKey, 0);
    return (hResult != NULL);
}

void IocpCore::Shutdown()
{
    if (!m_bIsRunning.load(std::memory_order_acq_rel))
        return;
    
    for (size_t i = 0; i < m_workerThreads.size(); i++)
        PostQueuedCompletionStatus(m_hIocp, 0, 0, nullptr);
    
    for (std::thread& thread : m_workerThreads)
    {
        if (thread.joinable())
            thread.join();
    }
    
    if (m_hIocp != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hIocp);
        m_hIocp = INVALID_HANDLE_VALUE;
    }
}
