#include "pch.h"
#include "ClientPacketHandler.h"

#include "Session.h"
#include "DBManager.h"
#include "GameSession.h"

std::unordered_map<uint16_t, std::function<void(Session*,char*)>> ClientPacketHandler::_packetHandlers;

void ClientPacketHandler::Init()
{
    _packetHandlers[(uint16_t)EPacketId::LoginReq] = Handle_LoginReq;
    _packetHandlers[(uint16_t)EPacketId::CreateNicknameReq] = Handle_CreateNicknameReq;
    _packetHandlers[(uint16_t)EPacketId::EnterGameReq] = Handle_EnterGameReq;
    _packetHandlers[(uint16_t)EPacketId::RefreshShopReq] = Handle_RefreshShopReq;
}

void ClientPacketHandler::HandlePacket(Session* session, char* packetData, uint16_t id)
{
    auto it = _packetHandlers.find(id);
    if (it != _packetHandlers.end())
    {
        it->second(session, packetData); 
    }
    else
    {
        GLOG(ClientPacketHandler, "등록되지 않은 패킷 수신! ID: %d", id);
    }
}

void ClientPacketHandler::Handle_LoginReq(Session* session, char* packetData)
{
    PKT_C2S_LoginReq* loginReq = (PKT_C2S_LoginReq*)packetData;
    GLOG(Session, "ID: %s / PW: %s", loginReq->username, loginReq->password);

    int32_t accountId = GDBManager->VerifyAccount(loginReq->username, loginReq->password);
    bool bSuccess = (accountId != -1);

    PKT_S2C_LoginRes loginRes;
    loginRes.header.size = sizeof(PKT_S2C_LoginRes);
    loginRes.header.id = (uint16_t)EPacketId::LoginRes;
    loginRes.bSuccess = bSuccess;
    loginRes.accountId = bSuccess ? accountId : 0;

    if (bSuccess)
    {
        session->SendAccountId(accountId);

        shared_ptr<Player> playerProfile = GDBManager->LoadPlayerProfile(accountId);

        if (playerProfile != nullptr)
        {
            loginRes.bHasProfile = true;
            strncpy_s(loginRes.nickname, playerProfile->GetNickname().c_str(), 31);
            GLOG(Session, "-> 기존 유저 접속! 닉네임: %s", loginRes.nickname);
        }
        else
        {
            loginRes.bHasProfile = false;
            memset(loginRes.nickname, 0, 32);
            GLOG(Session, "-> 신규 유저 접속! 닉네임 생성 요청 필요.");
        }
    }

    session->Send((char*)&loginRes, loginRes.header.size);

    if (bSuccess) GLOG(Session, "-> 로그인 성공! 클라이언트에 맵 이동 명령을 하달합니다.");
    else GLOG(Session, "-> 로그인 실패! 클라이언트에 에러를 보냅니다.");
}

void ClientPacketHandler::Handle_CreateNicknameReq(Session* session, char* packetData)
{
    PKT_C2S_CreateNicknameReq* req = (PKT_C2S_CreateNicknameReq*)packetData;
    GLOG(Session, "Session %llu 닉네임 생성 요청: %s", session->GetSessionId(), req->nickname);

    bool bSuccess = false;

    int32_t accountId = session->GetAccountId();

    if (accountId != 0)
    {
        bSuccess = GDBManager->CreatePlayerProfile(accountId, req->nickname);
    }
    else
    {
        GLOG_ERROR(Session, "로그인되지 않은 유저의 생성 요청입니다!");
    }

    PKT_S2C_CreateNicknameRes res;
    res.header.size = sizeof(PKT_S2C_CreateNicknameRes);
    res.header.id = (uint16_t)EPacketId::CreateNicknameRes;
    res.bSuccess = bSuccess;

    session->Send((char*)&res, res.header.size);
    if (bSuccess) GLOG(Session, "-> 닉네임 생성 및 DB 저장 성공!");
}

void ClientPacketHandler::Handle_EnterGameReq(Session* session, char* packetData)
{
    PKT_C2S_EnterGameReq* req = (PKT_C2S_EnterGameReq*)packetData;
    GLOG(Session, "게임 시작 요청");
    
    auto newGameSession = std::make_unique<GameSession>(session);
    GameSession* gamePtr = newGameSession.get();
    session->SetGameSession(std::move(newGameSession));

    bool bSuccess = true;

    PKT_S2C_EnterGameRes res;
    res.header.size = sizeof(PKT_S2C_EnterGameRes);
    res.header.id = (uint16_t)EPacketId::EnterGameRes;
    res.bSuccess = bSuccess;
    
    gamePtr->InitGame();
    session->Send((char*)&res, res.header.size);
    if (bSuccess) GLOG(Session, "게임 시작 성공!");
}

void ClientPacketHandler::Handle_RefreshShopReq(Session* session, char* packetData)
{
    GLOG(Session, "Session %llu 상점 리롤 요청 수신!", session->GetSessionId());

    GameSession* game = session->GetGameSession();
    
    if (game != nullptr)
    {
        game->RefreshShop();
    }
    else
    {
        GLOG_ERROR(Session, "게임 방에 입장하지 않은 유저의 리롤 요청입니다!");
    }
}