#pragma once

#include <vector>
#include <list>
#include <functional>
#include <mutex>
#include <atomic>
#include "JobQueue.h"

using TimerCallback = std::function<void()>;

class TimerWheel
{
private:
    std::vector<std::list<TimerCallback>> m_wheel;
    std::atomic<uint64_t> m_uCurrentTick;
    std::mutex m_wheelLock;
    JobQueue* m_pJobQueue;
    size_t m_wheelSize;

public:
    TimerWheel(size_t wheelSize, JobQueue* jobQueue);
    ~TimerWheel();

    void AddTimer(uint32_t delayTicks, TimerCallback callback);
    void Tick();
    
    uint64_t GetCurrentTick() const { return m_uCurrentTick.load(); }
};
