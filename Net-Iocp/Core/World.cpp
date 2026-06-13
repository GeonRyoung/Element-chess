#include "World.h"
#include <cmath>

World::World(float width, float height, float sectorSize)
    : m_fWorldWidth(width), m_fWorldHeight(height), m_fSectorSize(sectorSize)
{
    int numCols = static_cast<int>(std::ceil(width / sectorSize));
    int numRows = static_cast<int>(std::ceil(height / sectorSize));

    m_sectors.resize(numRows);
    uint32_t sectorId = 0;
    for (int r = 0; r < numRows; ++r)
    {
        m_sectors[r].resize(numCols);
        for (int c = 0; c < numCols; ++c)
        {
            m_sectors[r][c] = std::make_shared<Sector>(sectorId++);
        }
    }
}

std::shared_ptr<Sector> World::GetSector(float x, float y)
{
    if (x < 0.0f) x = 0.0f;
    if (x >= m_fWorldWidth) x = m_fWorldWidth - 1.0f;
    if (y < 0.0f) y = 0.0f;
    if (y >= m_fWorldHeight) y = m_fWorldHeight - 1.0f;

    int col = static_cast<int>(x / m_fSectorSize);
    int row = static_cast<int>(y / m_fSectorSize);

    return m_sectors[row][col];
}

void World::AddEntity(std::shared_ptr<Entity> entity)
{
    if (!entity) return;

    {
        std::unique_lock<std::shared_mutex> lock(m_worldLock);
        m_entityMap[entity->GetEntityId()] = entity;
    }

    Vector2 pos = entity->GetPosition();
    std::shared_ptr<Sector> sector = GetSector(pos.x, pos.y);
    
    if (sector)
    {
        entity->SetCurrentSector(sector);
        sector->AddEntity(entity);
    }
}

void World::RemoveEntity(uint32_t entityId)
{
    std::shared_ptr<Entity> entity;
    {
        std::unique_lock<std::shared_mutex> lock(m_worldLock);
        auto it = m_entityMap.find(entityId);
        if (it != m_entityMap.end())
        {
            entity = it->second;
            m_entityMap.erase(it);
        }
    }

    if (entity)
    {
        std::shared_ptr<Sector> sector = entity->GetCurrentSector();
        if (sector)
        {
            sector->RemoveEntity(entity);
        }
        entity->SetCurrentSector(nullptr);
    }
}

void World::MoveEntity(std::shared_ptr<Entity> entity, Vector2 newPos)
{
    if (!entity) return;

    std::shared_ptr<Sector> oldSector = entity->GetCurrentSector();
    entity->SetPosition(newPos);
    std::shared_ptr<Sector> newSector = GetSector(newPos.x, newPos.y);

    if (oldSector != newSector)
    {
        if (oldSector) oldSector->RemoveEntity(entity);
        if (newSector) newSector->AddEntity(entity);
        entity->SetCurrentSector(newSector);
    }
}

void World::BroadcastToAOI(std::shared_ptr<Entity> sender, std::shared_ptr<Packet> packet, float aoiRadius)
{
    if (!sender || !packet) return;

    Vector2 centerPos = sender->GetPosition();
    
    // Bounding Box를 기반으로 탐색할 섹터의 행/열 범위 계산
    int minCol = static_cast<int>(std::max(0.0f, centerPos.x - aoiRadius) / m_fSectorSize);
    int maxCol = static_cast<int>(std::min(m_fWorldWidth - 1.0f, centerPos.x + aoiRadius) / m_fSectorSize);
    int minRow = static_cast<int>(std::max(0.0f, centerPos.y - aoiRadius) / m_fSectorSize);
    int maxRow = static_cast<int>(std::min(m_fWorldHeight - 1.0f, centerPos.y + aoiRadius) / m_fSectorSize);

    float aoiRadiusSq = aoiRadius * aoiRadius;

    // 인접 섹터들만 순회
    for (int r = minRow; r <= maxRow; ++r)
    {
        for (int c = minCol; c <= maxCol; ++c)
        {
            std::shared_ptr<Sector> targetSector = m_sectors[r][c];
            if (!targetSector) continue;

            auto entities = targetSector->GetEntitiesSnapshot();
            for (const auto& entity : entities)
            {
                if (!entity || !entity->IsPlayer() || entity == sender)
                    continue;

                Vector2 ePos = entity->GetPosition();
                float dx = ePos.x - centerPos.x;
                float dy = ePos.y - centerPos.y;
                float distSq = dx * dx + dy * dy;

                if (distSq <= aoiRadiusSq)
                {
                    entity->SendPacket(packet);
                }
            }
        }
    }
}

void World::UpdateWorldTick(float deltaTime)
{
    std::vector<std::shared_ptr<Entity>> currentEntities;
    {
        std::shared_lock<std::shared_mutex> lock(m_worldLock);
        currentEntities.reserve(m_entityMap.size());
        for (const auto& pair : m_entityMap)
        {
            currentEntities.push_back(pair.second);
        }
    }

    for (auto& entity : currentEntities)
    {
        if (entity)
        {
            entity->Update(deltaTime);
        }
    }
}
