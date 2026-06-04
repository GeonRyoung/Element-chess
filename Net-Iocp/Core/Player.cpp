#include "Player.h"
#include "RdupSession.h"
#include "Packet.h"

Player::Player(uint32_t entityId, std::shared_ptr<RudpSession> session, const std::string& username)
    : Entity(entityId), m_pSession(session), m_strUsername(username)
{
}

void Player::SendPacket(std::shared_ptr<Packet> packet)
{
    if (auto session = m_pSession.lock())
    {
        session->SendPacket(packet);
    }
}

void Player::Update(float deltaTime)
{
    // 플레이어의 주기적인 상태 갱신 로직 (예: 스태미나 회복 등)
}
