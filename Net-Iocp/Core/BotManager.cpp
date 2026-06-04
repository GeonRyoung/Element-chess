#include "BotManager.h"

std::shared_ptr<Bot> BotManager::CreateBot(uint32_t entityId, Vector2 spawnPos)
{
    std::lock_guard<std::mutex> lock(m_botLock);
    
    auto bot = std::make_shared<Bot>(entityId);
    bot->SetPosition(spawnPos);
    m_bots[entityId] = bot;
    
    return bot;
}

void BotManager::RemoveBot(uint32_t entityId)
{
    std::lock_guard<std::mutex> lock(m_botLock);
    m_bots.erase(entityId);
}

void BotManager::UpdateBots(float deltaTime)
{
    // 월드 틱과는 별개로 봇 스폰/디스폰 등의 매니저 레벨 업데이트 처리
    // 실제 봇 객체(Entity)의 Update(FSM)는 World::UpdateWorldTick()에 의해 갱신됩니다.
}
