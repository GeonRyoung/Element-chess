#include "GameLogic.h"
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

    // TODO: 월드 시뮬레이션 틱 갱신 (3단계 World 구현 후 연동)
    // if (m_pWorld) {
    //     m_pWorld->UpdateWorldTick(deltaTime);
    // }
}
