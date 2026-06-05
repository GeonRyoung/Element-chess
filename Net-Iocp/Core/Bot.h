#pragma once
#include "Entity.h"
#include "BotState.h"

class Bot : public Entity
{
private:
    BotState m_eState;
    Vector2 m_vTargetPosition;
    float m_fStateTimer;

public:
    Bot(uint32_t entityId);
    virtual ~Bot() = default;

    virtual void Update(float deltaTime) override;
    void ChangeState(BotState newState);

private:
    void ProcessIdle(float deltaTime);
    void ProcessMove(float deltaTime);
    void ProcessAttack(float deltaTime);
};
