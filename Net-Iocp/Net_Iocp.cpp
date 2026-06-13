#include <iostream>
#include <thread>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Core/IocpCore.h"
#include "Core/SessionManager.h"
#include "Core/GameLogic.h"
#include "Core/World.h"
#include "Core/Player.h"
#include "Core/BotManager.h"
#include "Core/RecvContext.h"
#include "Core/TimerWheel.h"
#include "Core/MetricManager.h"
#include "Core/Logger.h"
#include "Core/httplib.h"

#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
    Logger::Info("Starting Net-Iocp Server...");

    // 0. Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        Logger::Error("WSAStartup failed.");
        return -1;
    }

    // 1. IocpCore 및 시스템 매니저 초기화
    IocpCore& iocp = IocpCore::GetInstance();
    if (!iocp.CreateIocpPort())
    {
        Logger::Error("Failed to create IOCP port!");
        WSACleanup();
        return -1;
    }

    SessionManager& sessionManager = SessionManager::GetInstance();
    iocp.SetSessionManager(std::shared_ptr<SessionManager>(&sessionManager, [](SessionManager*){}));

    // 8. 서버 소켓 및 IOCP 초기화
    SOCKET serverSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "WSASocket failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // 포트 재사용 옵션 설정
    BOOL optval = TRUE;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
    
    // SIO_UDP_CONNRESET 방지 (ICMP 에러로 인한 수신 중단 방지)
    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(serverSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000); // 9000 포트 고정
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    sessionManager.SetServerSocket(serverSocket);

    // Completion Key 0으로 등록 (서버 수신 전용)
    iocp.RegisterSocket(serverSocket, 0);

    // 9. 초기 RecvContext 투척 (여러 개를 투척하여 동시 다발적인 UDP 패킷 수신 처리)
    constexpr int NUM_RECV_CONTEXTS = 100;
    std::vector<std::unique_ptr<RecvContext>> recvContexts;
    for (int i = 0; i < NUM_RECV_CONTEXTS; ++i)
    {
        auto ctx = std::make_unique<RecvContext>();
        ctx->Init();
        sessionManager.PostRecvFrom(ctx.get());
        recvContexts.push_back(std::move(ctx));
    }

    std::cout << "Server Initialized. Listening on UDP port 9000..." << std::endl;
    std::cout << "RUDP Multiplexing Enabled. Bot Traffic starting..." << std::endl;

    // 10. 메인 루프 (10초마다 상태 출력)
    std::shared_ptr<World> world = std::make_shared<World>(1000.0f, 1000.0f, 100.0f);

    // 3. GameLogic 초기화
    GameLogic gameLogic;
    gameLogic.SetWorld(world);
    
    // 4. TimerWheel 초기화 (60슬롯, 1초마다 한바퀴 도는 구조로 가정, 슬롯 하나당 16.6ms)
    TimerWheel timerWheel(60, gameLogic.GetJobQueue());
    sessionManager.SetTimerWheel(&timerWheel);
    
    // 4.1 PacketDispatcher 스레드 연동 (IOCP 스레드 -> GameLogic 단일 스레드로 작업 이관)
    sessionManager.GetPacketDispatcher()->SetExecutionService(&gameLogic);
    
    // 4.5 패킷 핸들러 등록 (CS_POSITION_UPDATE)
    sessionManager.GetPacketDispatcher()->RegisterHandler(
        static_cast<uint16_t>(PacketOpcode::CS_POSITION_UPDATE),
        [world](std::shared_ptr<RudpSession> session, PacketPtr packet) {
            CS_PositionBody body;
            if (packet->Read(body))
            {
                uint32_t entityId = static_cast<uint32_t>(session->GetSessionId());
                std::shared_ptr<Entity> entity = world->GetEntity(entityId);
                if (!entity)
                {
                    entity = std::make_shared<Player>(entityId, session, "User");
                    entity->SetPosition(Vector2{body.x, body.y});
                    world->AddEntity(entity);
                }
                
                world->MoveEntity(entity, Vector2{body.x, body.y});
                
                PacketPtr broadcastPacket = SessionManager::GetInstance().AllocPacket();
                broadcastPacket->Init(PacketOpcode::SC_POSITION_UPDATE, 0);
                SC_PositionBody scBody;
                scBody.sessionId = entityId;
                scBody.x = body.x;
                scBody.y = body.y;
                broadcastPacket->Write(scBody);
                
                world->BroadcastToAOI(entity, broadcastPacket, 50.0f); // 반경 50 범위 내 브로드캐스트
            }
        });

    // 5. BotManager 봇 스폰 테스트 (더미 클라이언트 접속 테스트용)
    BotManager botManager;
    botManager.SpawnBots(100, world); // 100마리 더미 스폰

    // 6. HTTP 모니터링 서버(cpp-httplib) 구동 (백그라운드 스레드)
    std::thread httpThread([]() {
        httplib::Server svr;
        svr.Get("/metrics", [](const httplib::Request& /*req*/, httplib::Response& res) {
            std::string metricsData = MetricManager::GetInstance().SerializeMetrics();
            res.set_content(metricsData, "text/plain");
        });
        
        Logger::Info("Monitoring HTTP server listening on port 8080 (http://localhost:8080/metrics)");
        svr.listen("0.0.0.0", 8080);
    });
    httpThread.detach();

    Logger::Info("Server initialization complete. Listening on UDP 9000. Entering main loop.");

    // 7. 메인 게임 루프 (Tick 통합)
    // 60FPS (약 16.6ms) 간격으로 Update 실행
    const auto TICK_INTERVAL = std::chrono::milliseconds(16);
    auto lastLogTime = std::chrono::high_resolution_clock::now();
    
    while (iocp.IsRunning())
    {
        auto start = std::chrono::high_resolution_clock::now();

        // 6.1 TimerWheel 틱 업데이트
        timerWheel.Tick();

        // 6.2 게임 로직(물리, 상태) 업데이트
        gameLogic.Update(0.016f); // deltaTime

        // 6.3 메트릭 업데이트
        MetricManager::GetInstance().SetSystemMetrics(
            sessionManager.GetActiveSessionCount(),
            sessionManager.GetSessionPoolAvailableCount()
        );

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 6.4 주기적 상태 출력 (10초)
        if (std::chrono::duration_cast<std::chrono::seconds>(end - lastLogTime).count() >= 10)
        {
            lastLogTime = end;
            Logger::Info("=== [Status Report] Active Sessions: " + std::to_string(sessionManager.GetActiveSessionCount()) + 
                         " | Pool Available: " + std::to_string(sessionManager.GetSessionPoolAvailableCount()) + " ===");
        }

        // 6.5 남은 시간만큼 대기하여 루프 주기 맞춤
        if (elapsed < TICK_INTERVAL)
        {
            std::this_thread::sleep_for(TICK_INTERVAL - elapsed);
        }
    }

    Logger::Info("Server shutting down...");
    
    iocp.Shutdown();
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
