#include "pch.h"
#include "DBManager.h"

/*=====================
    DB 연결
=====================*/

bool DBManager::Connect(const string& host, int port, const string& user, const string& password, const string& schemaName) {
    try
    {
        _session = make_unique<mysqlx::Session>(host, port, user, password);
        _schemaName = schemaName;

        cout << " [DBManager] Successfully connected to database: " << schemaName << endl;
        return true;
    }
    catch (const mysqlx::Error& err)
    {
        cerr << " [DBManager] Connection Error: " << err.what() << endl;
        return false;
    }
    catch (const std::exception& ex)
    {
        cerr << " [DBManager] System Error: " << ex.what() << endl;
        return false;
    }
}

void DBManager::Disconnect()
{
    if (_session)
    {
        try
        {
            _session->close();
            _session.reset();
            cout << " [DBManager] Disconnected from database." << endl;
        }
        catch (...)  {}
    }
}

/*=====================
    로그인
=====================*/

int32_t DBManager::VerifyAccount(const string& username, const string& passwordHash) {
    try {
        auto table = GetSchema().getTable("user_account");
        auto res = table.select("account_id", "password_hash").where("username = :name")
            .bind("name", username).execute();
        auto row = res.fetchOne();
        if (row && (string)row[1] == passwordHash) 
            return (int32_t)row[0];
    }
    catch (...) {}
    return -1;
}

shared_ptr<Player> DBManager::LoadPlayerProfile(int32_t accountId) {
    try {
        auto table = GetSchema().getTable("player_profile");
        auto res = table.select("profile_id", "nickname", "gold", "player_level")
            .where("account_id = :id").bind("id", accountId).execute();
        auto row = res.fetchOne();
        if (row) 
            return make_shared<Player>((int32_t)row[0], (string)row[1], (int32_t)row[2], (int32_t)row[3]);
    }
    catch (...) {}
    return nullptr;
}
