#pragma once
#include <unordered_map>
class World;
#include <mutex>
#include <memory>
#include <cstdint>
#include "Bot.h"

class BotManager
{
private:
    std::unordered_map<uint32_t, std::shared_ptr<Bot>> m_bots;
    std::mutex m_botLock;

public:
    BotManager() = default;
    ~BotManager() = default;

    std::shared_ptr<Bot> CreateBot(uint32_t entityId, Vector2 spawnPos);
    void RemoveBot(uint32_t entityId);
    void UpdateBots(float deltaTime);
    
    void SpawnBots(int count, std::shared_ptr<World> world);
    void RemoveAllBots();
    size_t GetBotCount() const;
};
