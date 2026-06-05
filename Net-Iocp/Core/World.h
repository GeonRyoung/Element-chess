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
    void AddEntity(std::shared_ptr<Entity> entity);
    void RemoveEntity(uint32_t entityId);
    void MoveEntity(std::shared_ptr<Entity> entity, Vector2 newPos);
    void UpdateWorldTick(float deltaTime);
};
