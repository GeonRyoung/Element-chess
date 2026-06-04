#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <chrono>

using JobFunc = std::function<void()>;

class JobQueue
{
private:
    std::queue<JobFunc> m_jobQueue;
    std::mutex m_queueLock;
    std::condition_variable m_conditionVar;

public:
    JobQueue() = default;
    ~JobQueue() = default;

    void PushJob(JobFunc job)
    {
        {
            std::lock_guard<std::mutex> lock(m_queueLock);
            m_jobQueue.push(job);
        }
        m_conditionVar.notify_one();
    }

    void PopAllJobs(std::vector<JobFunc>& jobs)
    {
        std::lock_guard<std::mutex> lock(m_queueLock);
        while (!m_jobQueue.empty())
        {
            jobs.push_back(std::move(m_jobQueue.front()));
            m_jobQueue.pop();
        }
    }

    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(m_queueLock);
        return m_jobQueue.empty();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_queueLock);
        m_conditionVar.wait(lock, [this]() { return !m_jobQueue.empty(); });
    }

    template <typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period>& rel_time)
    {
        std::unique_lock<std::mutex> lock(m_queueLock);
        return m_conditionVar.wait_for(lock, rel_time, [this]() { return !m_jobQueue.empty(); });
    }
};
