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
		const string& password, const string& schema);
	
	void Disconnect();

	/*=====================
		로그인
	=====================*/

	int32_t VerifyAccount(const string& username, const string& passwordHash);
	shared_ptr<Player> LoadPlayerProfile(int32_t accountId);

	mysqlx::Schema GetSchema() {
		if (_session) {
			return _session->getSchema(_schemaName);
		}
		throw std::runtime_error("DB Session is not connected!");
	}

private:
	DBManager() = default;
	~DBManager() { Disconnect(); }

 	unique_ptr<mysqlx::Session> _session = nullptr;
	std::string _schemaName = "";
};

