#pragma once

class Session;

class GameSession
{
	GameSession(std::shared_ptr<Session> ownerSession);
	~GameSession();

	void InitGame();
	void EndGame();


private:
	std::weak_ptr<Session> _ownerSession;

	int32_t _playerHp = 100;
	int32_t _gold = 0;
	int32_t _level = 1;
	int32_t currentWave = 1;

	std::unordered_map<int32_t, int32_t> _myShopPool;
	std::unordered_map<int32_t, int32_t> _barodGrid;
};

