#pragma once
#include "Player.h"

class DBManager
{
public:
	static DBManager* GetInstance()
	{
		static DBManager instance;
		return &instance;
	}

	/*=====================
		DB 연결
	=====================*/

	bool Connect(const string& host, int port, const string& user,
		const string& password, const string& schema, int32_t poolSize = 10);
	
	void Disconnect();

	/*=====================
		커넥션 대여 및 반납 
	=====================*/
	MYSQL* PopConnection();
	void PushConnection(MYSQL* conn);
	
	
	/*=====================
		로그인
	=====================*/

	int32_t VerifyAccount(const string& username, const string& passwordHash);
	shared_ptr<Player> LoadPlayerProfile(int32_t accountId);
	bool CreatePlayerProfile(int32_t accountId, const string& nickname);

private:
	DBManager() = default;
	~DBManager() { Disconnect(); }

	std::queue<MYSQL*> _connectionPool;
	std::mutex _poolLock;
	std::condition_variable _poolCv;
};

class DBConnectionGuard
{
public:
	DBConnectionGuard() 
	{ 
		_conn = DBManager::GetInstance()->PopConnection(); 
	}
	~DBConnectionGuard() 
	{ 
		if (_conn) DBManager::GetInstance()->PushConnection(_conn); 
	}
	MYSQL* Get() { return _conn; }

private:
	MYSQL* _conn = nullptr;
};