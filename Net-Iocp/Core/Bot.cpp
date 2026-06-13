#include "Bot.h"
#include <cmath>
#include <WS2tcpip.h>
#include "Packet.h"

Bot::Bot(uint32_t entityId)
    : Entity(entityId), m_eState(BotState::IDLE), m_vTargetPosition{0.0f, 0.0f}, m_fStateTimer(0.0f), m_uSequence(1)
{
    m_clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_clientSocket != INVALID_SOCKET)
    {
        u_long mode = 1;
        ioctlsocket(m_clientSocket, FIONBIO, &mode);
        
        ZeroMemory(&m_serverAddr, sizeof(m_serverAddr));
        m_serverAddr.sin_family = AF_INET;
        m_serverAddr.sin_port = htons(9000);
        inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr);
    }
}

Bot::~Bot()
{
    if (m_clientSocket != INVALID_SOCKET)
    {
        closesocket(m_clientSocket);
    }
}

void Bot::Update(float deltaTime)
{
    switch (m_eState)
    {
    case BotState::IDLE:
        ProcessIdle(deltaTime);
        break;
    case BotState::MOVE:
        ProcessMove(deltaTime);
        break;
    case BotState::ATTACK:
        ProcessAttack(deltaTime);
        break;
    case BotState::DEAD:
        break;
    }
}

void Bot::ChangeState(BotState newState)
{
    m_eState = newState;
    m_fStateTimer = 0.0f;
}

void Bot::ProcessIdle(float deltaTime)
{
    m_fStateTimer += deltaTime;
    if (m_fStateTimer > 3.0f)
    {
        // 3초 대기 후 이동 상태로 전환
        ChangeState(BotState::MOVE);
        // 임의의 타겟 위치 설정 (간단한 예시)
        m_vTargetPosition.x = m_vPosition.x + 10.0f;
        m_vTargetPosition.y = m_vPosition.y + 10.0f;
    }
}

void Bot::ProcessMove(float deltaTime)
{
    float dx = m_vTargetPosition.x - m_vPosition.x;
    float dy = m_vTargetPosition.y - m_vPosition.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f)
    {
        ChangeState(BotState::ATTACK);
    }
    else
    {
        float speed = 5.0f;
        Vector2 newPos;
        newPos.x = m_vPosition.x + (dx / dist) * speed * deltaTime;
        newPos.y = m_vPosition.y + (dy / dist) * speed * deltaTime;
        SetPosition(newPos);
        
        SendPacket(0x0101); // CS_POSITION_UPDATE
    }
}

void Bot::ProcessAttack(float deltaTime)
{
    m_fStateTimer += deltaTime;
    if (m_fStateTimer > 1.0f)
    {
        // 공격 후 다시 대기
        ChangeState(BotState::IDLE);
    }
}

void Bot::SendPacket(uint16_t opcode)
{
    if (m_clientSocket == INVALID_SOCKET) return;
    
    Packet packet;
    packet.Init(static_cast<PacketOpcode>(opcode), m_uSequence++);
    
    if (opcode == 0x0101) // CS_POSITION_UPDATE
    {
        CS_PositionBody body;
        body.x = GetPosition().x;
        body.y = GetPosition().y;
        packet.Write(body);
    }
    
    sendto(m_clientSocket, packet.GetBuffer(), packet.GetTotalSize(), 0, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr));
}
