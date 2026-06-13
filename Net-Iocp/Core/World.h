#pragma once
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include "Sector.h"
#include "Entity.h"

class World : public std::enable_shared_from_this<World>
{
private:
    std::vector<std::vector<std::shared_ptr<Sector>>> m_sectors;
    std::unordered_map<uint32_t, std::shared_ptr<Entity>> m_entityMap;
    std::shared_mutex m_worldLock;

    float m_fWorldWidth;
    float m_fWorldHeight;
    float m_fSectorSize;

public:
    World(float width, float height, float sectorSize);
    ~World() = default;

    std::shared_ptr<Sector> GetSector(float x, float y);
    
    std::shared_ptr<Entity> GetEntity(uint32_t entityId)
    {
        std::shared_lock<std::shared_mutex> lock(m_worldLock);
        auto it = m_entityMap.find(entityId);
        if (it != m_entityMap.end()) return it->second;
        return nullptr;
    }
    
    void AddEntity(std::shared_ptr<Entity> entity);
    void RemoveEntity(uint32_t entityId);
    void MoveEntity(std::shared_ptr<Entity> entity, Vector2 newPos);
    
    // 시야 반경(AOI) 내의 플레이어들에게만 선별적으로 패킷 전송
    void BroadcastToAOI(std::shared_ptr<Entity> sender, std::shared_ptr<class Packet> packet, float aoiRadius);
    
    void UpdateWorldTick(float deltaTime);
};
