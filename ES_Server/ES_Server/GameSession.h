#pragma once

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
	int32_t DrawUnitFromPool(int32_t playerLevel);
	
	Session* _ownerSession;

	int32_t _playerHp = 100;
	int32_t _gold = 0;
	int32_t _level = 1;
	int32_t currentWave = 1;

	std::unordered_map<int32_t, int32_t> _myShopPool;
	std::unordered_map<int32_t, int32_t> _barodGrid;
	
	std::unordered_map<int32_t, int32_t> _remainUnitCounts;
};

