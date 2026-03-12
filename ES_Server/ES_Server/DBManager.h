#pragma once

class DBManager
{
public:
	static DBManager* GetInstance()
	{
		static DBManager instance;
		return &instance;
	}

	bool Connect(const string& host, int port, const string& user,
		const string& password, const string& schema);
	
	void Disconnect();

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

