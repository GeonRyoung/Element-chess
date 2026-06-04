#include "Sector.h"

Sector::Sector(uint32_t sectorId) : m_uSectorId(sectorId)
{
}

void Sector::AddEntity(std::shared_ptr<Entity> entity)
{
    if (!entity) return;

    std::unique_lock<std::shared_mutex> lock(m_sectorLock);
    m_entities.insert(entity);
}

void Sector::RemoveEntity(std::shared_ptr<Entity> entity)
{
    if (!entity) return;

    std::unique_lock<std::shared_mutex> lock(m_sectorLock);
    m_entities.erase(entity);
}

void Sector::Broadcast(std::shared_ptr<Packet> packet)
{
    if (!packet) return;

    std::shared_lock<std::shared_mutex> lock(m_sectorLock);
    for (const auto& entity : m_entities)
    {
        if (entity && entity->IsPlayer())
        {
            entity->SendPacket(packet);
        }
    }
}
