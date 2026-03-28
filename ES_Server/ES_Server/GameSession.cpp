#include "pch.h"
#include "GameSession.h"

#include "Session.h"

GameSession::GameSession(Session* ownerSession)
{
    _ownerSession = ownerSession;
    _gold = 10; // 시작 골드
    cout << "[GameSession] 새로운 게임 방이 생성되었습니다!" << endl;
}

GameSession::~GameSession()
{
    std::cout << "[GameSession] 게임이 소멸되었습니다." << endl;
}

void GameSession::InitGame()
{
    std::vector<int32_t> tier1 = { 111,121,131, 211,221,231, 311,321,331, 411,421,431, 511,521,531, 611,621,631 };
    for (int32_t id : tier1) {
        _remainUnitCounts[id] = 9;
    }
    cout << "[GameSession] InitGame 완료: 상점 공유 풀 세팅 끝!" << endl;
    
    PKT_S2C_RefreshShopRes res;
    res.header.size = sizeof(PKT_S2C_RefreshShopRes);
    res.header.id = (uint16_t)EPacketId::RefreshShopRes;
    
    res.bSuccess = true;
    res.remainGold = _gold; 

    cout << " -> 첫 상점 무료 지급! 뽑힌 유닛: ";
    for (int i = 0; i < 5; i++)
    {
        res.shopUnits[i] = DrawUnitFromPool(_level); 
        cout << res.shopUnits[i] << " ";
    }
    cout << "\n";

    _ownerSession->Send((char*)&res, res.header.size);
}

void GameSession::EndGame()
{
}

void GameSession::RefreshShop()
{
    PKT_S2C_RefreshShopRes res;
    res.header.size = sizeof(PKT_S2C_RefreshShopRes);
    res.header.id = (uint16_t)EPacketId::RefreshShopRes;

    int32_t rerollCost = 2;

    if (_gold >= rerollCost)
    {
        res.bSuccess = true;
        _gold -= rerollCost; 
        res.remainGold = _gold;

        cout << " -> 리롤 성공! 남은 골드: " << res.remainGold << " / 뽑힌 유닛: ";
        for (int i = 0; i < 5; i++)
        {
            res.shopUnits[i] = DrawUnitFromPool(_level); 
            cout << res.shopUnits[i] << " ";
        }
        cout << "\n";
    }
    else
    {
        cout << " -> 리롤 실패: 골드가 부족합니다!\n";
        res.bSuccess = false;
        memset(res.shopUnits, 0, sizeof(res.shopUnits));
        res.remainGold = _gold; 
    }

    _ownerSession->Send((char*)&res, res.header.size);
}

int32_t GameSession::DrawUnitFromPool(int32_t playerLevel)
{
    std::vector<int32_t> lotteryBox;
    
    for (const auto& pair : _remainUnitCounts)
    {
        int32_t unitId = pair.first;
        int32_t remainCount = pair.second;
        
        if (remainCount > 0)
        {
            for (int i = 0; i < remainCount; ++i)
            {
                lotteryBox.push_back(unitId);
            }
        }
    }
    if (lotteryBox.empty()) return 0;
    int randomIndex = rand() % lotteryBox.size();
    return lotteryBox[randomIndex];
}

