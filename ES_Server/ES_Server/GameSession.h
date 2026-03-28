#pragma once

#include "random"

class Session;

class GameSession
{
public:
	GameSession(Session* ownerSession);
	~GameSession();

	void InitGame();
	void EndGame();

	void RefreshShop();
private:
	void UpdateLotteryBox();
	int32_t DrawUnitFromPool(int32_t playerLevel);
	
	Session* _ownerSession;

	int32_t _gold = 0;
	int32_t _level = 1;

	std::unordered_map<int32_t, int32_t> _myShopPool;
	std::unordered_map<int32_t, int32_t> _barodGrid;
	std::unordered_map<int32_t, int32_t> _remainUnitCounts;
	
	std::mt19937 _rng; 
	std::vector<int32_t> _lotteryBox;
};

