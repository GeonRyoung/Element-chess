#pragma once
#include "Entity.h"
#include <string>
#include <memory>

class RudpSession;

class Player : public Entity
{
private:
    std::weak_ptr<RudpSession> m_pSession;
    std::string m_strUsername;

public:
    Player(uint32_t entityId, std::shared_ptr<RudpSession> session, const std::string& username);
    virtual ~Player() = default;

    std::string GetUsername() const { return m_strUsername; }

    virtual bool IsPlayer() const override { return true; }
    virtual void SendPacket(std::shared_ptr<class Packet> packet) override;
    virtual void Update(float deltaTime) override;
};
