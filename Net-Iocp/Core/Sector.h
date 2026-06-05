#pragma once
#include <cstdint>
#include <unordered_set>
#include <shared_mutex>
#include <memory>
#include "Entity.h"
#include "Packet.h"

class Sector : public std::enable_shared_from_this<Sector>
{
private:
    uint32_t m_uSectorId;
    std::unordered_set<std::shared_ptr<Entity>> m_entities;
    std::shared_mutex m_sectorLock;

public:
    Sector(uint32_t sectorId);
    ~Sector() = default;

    uint32_t GetSectorId() const { return m_uSectorId; }

    void AddEntity(std::shared_ptr<Entity> entity);
    void RemoveEntity(std::shared_ptr<Entity> entity);
    
    // 섹터 내 플레이어에게만 패킷 브로드캐스트
    void Broadcast(std::shared_ptr<Packet> packet);
};
