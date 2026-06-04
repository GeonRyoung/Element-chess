#include "GameLogic.h"
#include "World.h"
#include <vector>

GameLogic::GameLogic() : m_pJobQueue(std::make_unique<JobQueue>())
{
}

void GameLogic::Execute(JobFunc job)
{
    if (m_pJobQueue)
    {
        m_pJobQueue->PushJob(job);
    }
}

void GameLogic::Update(float deltaTime)
{
    if (!m_pJobQueue)
        return;

    std::vector<JobFunc> jobs;
    m_pJobQueue->PopAllJobs(jobs);

    for (auto& job : jobs)
    {
        if (job)
        {
            job();
        }
    }

    if (m_pWorld) {
        m_pWorld->UpdateWorldTick(deltaTime);
    }
}
