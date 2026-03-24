#include "pch.h"
#include "GameSession.h"

GameSession::GameSession(std::shared_ptr<Session> ownerSession)
{
    _ownerSession = ownerSession;
    std::cout << "[GameSession] 새로운 게임이 생성되었습니다!" << std::endl;
}

GameSession::~GameSession()
{
    std::cout << "[GameSession] 게임이 소멸되었습니다." << std::endl;
}

void GameSession::InitGame()
{
}

void GameSession::EndGame()
{
}

