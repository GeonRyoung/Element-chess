#include "IocpCore.h"
#include <iostream>

#include "SessionManager.h"
#include "RdupSession.h"

void IocpCore::WorkerThreadMain()
{
    while (IsRunning())
    {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED pOverlapped = nullptr;

        BOOL bSuccess = GetQueuedCompletionStatus(
            m_hIocp,
            &bytesTransferred,
            &completionKey,
            &pOverlapped,
            INFINITE
        );

        if (!m_bIsRunning.load(std::memory_order_acquire))
            break;

        if (pOverlapped == nullptr)
            continue;

        if (m_sessionManager == nullptr)
            continue;

        const uint64_t sessionId = static_cast<uint64_t>(completionKey);
        std::shared_ptr<RudpSession> session = m_sessionManager->FindSession(sessionId);
        if (!session)
            continue;

        if (!bSuccess)
        {
            session->DisConnect();                                             
            m_sessionManager->RemoveSession(session->GetSessionId());           
            continue;
        }

        if (pOverlapped == session->GetRecvOverlappedPtr())                   
        {
            session->OnRecvCompleted(static_cast<size_t>(bytesTransferred));  

            if (session->GetState() == SessionState::Closed)                  
                m_sessionManager->RemoveSession(session->GetSessionId());       
        }
        else if (pOverlapped == session->GetSendOverlappedPtr())               
        {
            // TODO: 송신 완료 후 후속 처리(재전송 큐 제거/통계 반영 등)
            if (bytesTransferred == 0)
            {
                session->DisConnect();
                m_sessionManager->RemoveSession(session->GetSessionId());
            }
        }
        else
        {
            session->DisConnect();
            m_sessionManager->RemoveSession(session->GetSessionId());
        }
    }
}

bool IocpCore::CreateIocpPort(DWORD numThreads)
{
    if (m_bIsRunning.load(std::memory_order_acquire))                          
        return true;                                                        

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
        m_workerThreads.emplace_back([this]() { WorkerThreadMain(); });

    return true;
}

bool IocpCore::RegisterSocket(SOCKET socket, ULONG_PTR completionKey)
{
    HANDLE hResult = CreateIoCompletionPort(
        reinterpret_cast<HANDLE>(socket),
        m_hIocp,
        completionKey,
        0
    );
    return (hResult != NULL);
}

void IocpCore::Shutdown()
{
    bool expected = true;
    if (!m_bIsRunning.compare_exchange_strong(
        expected, false, std::memory_order_acq_rel))
        return;

    for (size_t i = 0; i < m_workerThreads.size(); i++)
        PostQueuedCompletionStatus(m_hIocp, 0, 0, nullptr);

    for (std::thread& thread : m_workerThreads)
    {
        if (thread.joinable())
            thread.join();
    }
    m_workerThreads.clear();

    if (m_hIocp != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hIocp);
        m_hIocp = INVALID_HANDLE_VALUE;
    }
}
