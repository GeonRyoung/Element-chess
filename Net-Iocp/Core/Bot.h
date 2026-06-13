#pragma once
#include "Entity.h"
#include "BotState.h"
#include <WinSock2.h>

class Bot : public Entity
{
private:
    BotState m_eState;
    Vector2 m_vTargetPosition;
    float m_fStateTimer;
    
    SOCKET m_clientSocket;
    SOCKADDR_IN m_serverAddr;
    uint32_t m_uSequence;

public:
    Bot(uint32_t entityId);
    virtual ~Bot();
    
    virtual void Update(float deltaTime) override;
    void ChangeState(BotState newState);

private:
    void ProcessIdle(float deltaTime);
    void ProcessMove(float deltaTime);
    void ProcessAttack(float deltaTime);
    
    void SendPacket(uint16_t opcode);
};
