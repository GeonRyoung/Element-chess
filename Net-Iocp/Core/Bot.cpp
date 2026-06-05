#include "Bot.h"
#include <cmath>

Bot::Bot(uint32_t entityId)
    : Entity(entityId), m_eState(BotState::IDLE), m_vTargetPosition{0.0f, 0.0f}, m_fStateTimer(0.0f)
{
}

void Bot::Update(float deltaTime)
{
    switch (m_eState)
    {
    case BotState::IDLE:
        ProcessIdle(deltaTime);
        break;
    case BotState::MOVE:
        ProcessMove(deltaTime);
        break;
    case BotState::ATTACK:
        ProcessAttack(deltaTime);
        break;
    case BotState::DEAD:
        break;
    }
}

void Bot::ChangeState(BotState newState)
{
    m_eState = newState;
    m_fStateTimer = 0.0f;
}

void Bot::ProcessIdle(float deltaTime)
{
    m_fStateTimer += deltaTime;
    if (m_fStateTimer > 3.0f)
    {
        // 3초 대기 후 이동 상태로 전환
        ChangeState(BotState::MOVE);
        // 임의의 타겟 위치 설정 (간단한 예시)
        m_vTargetPosition.x = m_vPosition.x + 10.0f;
        m_vTargetPosition.y = m_vPosition.y + 10.0f;
    }
}

void Bot::ProcessMove(float deltaTime)
{
    float dx = m_vTargetPosition.x - m_vPosition.x;
    float dy = m_vTargetPosition.y - m_vPosition.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f)
    {
        ChangeState(BotState::ATTACK);
    }
    else
    {
        float speed = 5.0f;
        Vector2 newPos;
        newPos.x = m_vPosition.x + (dx / dist) * speed * deltaTime;
        newPos.y = m_vPosition.y + (dy / dist) * speed * deltaTime;
        SetPosition(newPos);
    }
}

void Bot::ProcessAttack(float deltaTime)
{
    m_fStateTimer += deltaTime;
    if (m_fStateTimer > 1.0f)
    {
        // 공격 후 다시 대기
        ChangeState(BotState::IDLE);
    }
}
