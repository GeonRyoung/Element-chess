#pragma once
#include "IExecutionService.h"
#include "JobQueue.h"
#include <memory>

class World; // Forward declaration

class GameLogic : public IExecutionService
{
private:
    std::shared_ptr<World> m_pWorld;
    std::unique_ptr<JobQueue> m_pJobQueue;

public:
    GameLogic();
    virtual ~GameLogic() = default;

    virtual void Execute(JobFunc job) override;
    void Update(float deltaTime);

    std::shared_ptr<World> GetWorld() const { return m_pWorld; }
    void SetWorld(std::shared_ptr<World> world) { m_pWorld = world; }
    JobQueue* GetJobQueue() { return m_pJobQueue.get(); }
};
