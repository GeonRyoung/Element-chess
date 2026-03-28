#include "pch.h"
#include "GameSession.h"

#include "Session.h"

GameSession::GameSession(Session* ownerSession)
{
    _ownerSession = ownerSession;
    _gold = 10; 
    
    std::random_device rd;
    _rng.seed(rd());
    
    GLOG(GameSession, "새로운 게임 방이 생성되었습니다!");
}

GameSession::~GameSession()
{
    GLOG(GameSession, "게임이 소멸되었습니다.");
}

void GameSession::InitGame()
{
    std::vector<int32_t> tier1 = { 111,121,131, 211,221,231, 311,321,331, 411,421,431, 511,521,531, 611,621,631 };
    for (int32_t id : tier1) {
        _remainUnitCounts[id] = 9;
    }
    GLOG(GameSession, "InitGame 완료: 상점 공유 풀 세팅 끝!");
    
    UpdateLotteryBox();
    
    PKT_S2C_RefreshShopRes res;
    res.header.size = sizeof(PKT_S2C_RefreshShopRes);
    res.header.id = (uint16_t)EPacketId::RefreshShopRes;
    
    res.bSuccess = true;
    res.remainGold = _gold; 

    string unitsStr = "";
    for (int i = 0; i < 5; i++)
    {
        res.shopUnits[i] = DrawUnitFromPool(_level); 
        unitsStr += std::to_string(res.shopUnits[i]) + " ";
    }
    GLOG(GameSession, "-> 첫 상점 무료 지급! 뽑힌 유닛: %s", unitsStr.c_str());

    _ownerSession->Send((char*)&res, res.header.size);
}

void GameSession::UpdateLotteryBox()
{
    _lotteryBox.clear();
    for (const auto& pair : _remainUnitCounts)
    {
        for (int i = 0; i < pair.second; i++)
        {
            _lotteryBox.push_back(pair.first);
        }
    }
}

void GameSession::EndGame()
{
}

void GameSession::RefreshShop()
{
    PKT_S2C_RefreshShopRes res;
    res.header.size = sizeof(PKT_S2C_RefreshShopRes);
    res.header.id = (uint16_t)EPacketId::RefreshShopRes;
    res.remainGold = _gold;

    int32_t rerollCost = 2;

    if (_gold >= rerollCost)
    {
        res.bSuccess = true;
        _gold -= rerollCost; 
        res.remainGold = _gold;

        string unitsStr = "";
        for (int i = 0; i < 5; i++)
        {
            res.shopUnits[i] = DrawUnitFromPool(_level); 
            unitsStr += std::to_string(res.shopUnits[i]) + " ";
        }
        GLOG(GameSession, "-> 리롤 성공! 남은 골드: %d / 뽑힌 유닛: %s", res.remainGold, unitsStr.c_str());
    }
    else
    {
        GLOG_ERROR(GameSession, "리롤 실패: 골드가 부족합니다!");
        res.bSuccess = false;
        memset(res.shopUnits, 0, sizeof(res.shopUnits));
        res.remainGold = _gold; 
    }

    _ownerSession->Send((char*)&res, res.header.size);
}

int32_t GameSession::DrawUnitFromPool(int32_t playerLevel)
{
    if (_lotteryBox.empty()) return 0;
    
    std::uniform_int_distribution<int32_t> dist(0,static_cast<int32_t> (_lotteryBox.size() - 1));
    int32_t selectedUnitId = _lotteryBox[dist(_rng)];
    
    if (_remainUnitCounts[selectedUnitId] > 0)
    {
        _remainUnitCounts[selectedUnitId]--;
        UpdateLotteryBox(); 
    }

    return selectedUnitId;
}