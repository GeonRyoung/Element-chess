#include <iostream>
#include <thread>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Core/IocpCore.h"
#include "Core/SessionManager.h"
#include "Core/GameLogic.h"
#include "Core/World.h"
#include "Core/BotManager.h"
#include "Core/TimerWheel.h"
#include "Core/MetricManager.h"

#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
    std::cout << "Starting Net-Iocp Server..." << std::endl;

    // 0. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }

    // 1. IocpCore 및 시스템 매니저 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.CreateIocpPort())
    {
        std::cerr << "Failed to create IOCP port!" << std::endl;
        WSACleanup();
        return -1;
    }

    SessionManager& sessionManager = SessionManager::GetInstance();
    iocp.SetSessionManager(std::shared_ptr<SessionManager>(&sessionManager, [](SessionManager*){}));

    // 서버용 UDP 소켓 생성 및 바인딩
    SOCKET serverSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create UDP socket." << std::endl;
        iocp.Shutdown();
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(9000);

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        iocp.Shutdown();
        WSACleanup();
        return -1;
    }

    // 서버 소켓을 세션으로 래핑하고 IOCP에 등록
    auto serverSession = sessionManager.CreateSession(serverSocket);
    iocp.RegisterSocket(serverSocket, serverSession->GetSessionId());
    
    // 비동기 수신 시작
    serverSession->PostRecv();

    // 2. World 생성
    std::shared_ptr<World> world = std::make_shared<World>();

    // 3. GameLogic 초기화
    GameLogic gameLogic;
    gameLogic.SetWorld(world);

    // 4. TimerWheel 초기화 (60슬롯, 1초마다 한바퀴 도는 구조로 가정, 슬롯 하나당 16.6ms)
    TimerWheel timerWheel(60, gameLogic.GetJobQueue());

    // 5. BotManager 봇 스폰 테스트 (더미 클라이언트 접속 테스트용)
    BotManager botManager;
    botManager.SpawnBots(100, world); // 100마리 더미 스폰

    std::cout << "Server initialization complete. Listening on UDP 9000. Entering main loop." << std::endl;

    // 6. 메인 게임 루프 (Tick 통합)
    // 60FPS (약 16.6ms) 간격으로 Update 실행
    const auto TICK_INTERVAL = std::chrono::milliseconds(16);
    
    while (iocp.IsRunning())
    {
        auto start = std::chrono::high_resolution_clock::now();

        // 6.1 TimerWheel 틱 업데이트
        timerWheel.Tick();

        // 6.2 게임 로직(물리, 상태) 업데이트
        gameLogic.Update(0.016f); // deltaTime

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 6.3 남은 시간만큼 대기하여 루프 주기 맞춤
        if (elapsed < TICK_INTERVAL)
        {
            std::this_thread::sleep_for(TICK_INTERVAL - elapsed);
        }
    }

    std::cout << "Server shutting down..." << std::endl;
    
    iocp.Shutdown();
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
