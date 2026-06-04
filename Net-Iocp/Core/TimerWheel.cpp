#include "TimerWheel.h"

TimerWheel::TimerWheel(size_t wheelSize, JobQueue* jobQueue)
    : m_wheelSize(wheelSize), m_pJobQueue(jobQueue), m_uCurrentTick(0)
{
    m_wheel.resize(m_wheelSize);
}

TimerWheel::~TimerWheel()
{
}

void TimerWheel::AddTimer(uint32_t delayTicks, TimerCallback callback)
{
    std::lock_guard<std::mutex> lock(m_wheelLock);
    uint64_t targetTick = m_uCurrentTick.load() + delayTicks;
    size_t targetSlot = targetTick % m_wheelSize;
    m_wheel[targetSlot].push_back(std::move(callback));
}

void TimerWheel::Tick()
{
    std::list<TimerCallback> currentCallbacks;
    
    {
        std::lock_guard<std::mutex> lock(m_wheelLock);
        size_t currentSlot = m_uCurrentTick.load() % m_wheelSize;
        
        // 해당 슬롯의 모든 콜백을 추출하고 비움
        currentCallbacks = std::move(m_wheel[currentSlot]);
        m_wheel[currentSlot].clear();
        
        m_uCurrentTick.fetch_add(1);
    }
    
    // 추출한 콜백들을 JobQueue에 넣음 (락 범위를 벗어나서 수행하여 성능 최적화)
    if (m_pJobQueue && !currentCallbacks.empty())
    {
        for (auto& callback : currentCallbacks)
        {
            m_pJobQueue->PushJob(std::move(callback));
        }
    }
}
