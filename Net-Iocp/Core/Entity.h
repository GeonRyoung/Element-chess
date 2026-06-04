#pragma once
#include <memory>
#include <shared_mutex>
#include <cstdint>

struct Vector2
{
    float x;
    float y;
};

class Sector;

class Entity : public std::enable_shared_from_this<Entity>
{
protected:
    uint32_t m_uEntityId;
    Vector2 m_vPosition;
    std::weak_ptr<Sector> m_pCurrentSector;
    mutable std::shared_mutex m_stateLock;

public:
    Entity(uint32_t entityId) : m_uEntityId(entityId), m_vPosition{0.0f, 0.0f} {}
    virtual ~Entity() = default;

    uint32_t GetEntityId() const { return m_uEntityId; }
    
    Vector2 GetPosition() const
    {
        std::shared_lock<std::shared_mutex> lock(m_stateLock);
        return m_vPosition;
    }
    
    void SetPosition(Vector2 pos)
    {
        std::unique_lock<std::shared_mutex> lock(m_stateLock);
        m_vPosition = pos;
    }

    std::shared_ptr<Sector> GetCurrentSector() const
    {
        std::shared_lock<std::shared_mutex> lock(m_stateLock);
        return m_pCurrentSector.lock();
    }

    void SetCurrentSector(std::shared_ptr<Sector> sector)
    {
        std::unique_lock<std::shared_mutex> lock(m_stateLock);
        m_pCurrentSector = sector;
    }

    virtual void Update(float deltaTime) = 0;
    
    // Sector::Broadcast 등에서 의존성을 낮추기 위한 가상 함수
    virtual bool IsPlayer() const { return false; }
    virtual void SendPacket(std::shared_ptr<class Packet> packet) {}
};
