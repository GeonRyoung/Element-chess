#include "pch.h"
#include "DBManager.h"

/*=====================
    DB 연결
=====================*/

bool DBManager::Connect(const string& host, int port, const string& user, const string& password, const string& schemaName) {
    
    _conn = mysql_init(nullptr);
    if (_conn == nullptr)
    {
        cerr << "[DBManager] mysql_init failed!\n";
        return false;
    }

    if (mysql_real_connect(_conn, host.c_str(), user.c_str(), password.c_str(),
        schemaName.c_str(), port, nullptr, 0) == 0)
    {
        mysql_close(_conn);
        _conn = nullptr;
        return false;
    }   

    mysql_set_character_set(_conn, "utf8mb4");

    cout << " [DBManager] Successfully connected to database: " << schemaName << endl;
    return true;
}

void DBManager::Disconnect()
{
    if (_conn)
    {
        mysql_close(_conn);
        _conn = nullptr;
        cout << " [DBManager] Disconnected from database." << endl;
    }
}

/*=====================
    로그인
=====================*/

int32_t DBManager::VerifyAccount(const string& username, const string& passwordHash) {
    std::lock_guard<std::mutex> lock(_dbLock);

    if (!_conn) return -1;

    string query = std::format("SELECT account_id, password_hash FROM user_account WHERE username = '{}'", username);

    if (mysql_query(_conn, query.c_str()) == 0)
    {
        MYSQL_RES* result = mysql_store_result(_conn);
        if (result)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr)
            {
                string dpPassword = row[1] ? row[1] : "";
                if (dpPassword == passwordHash)
                {
                    int32_t accountId = std::stoi(row[0]);
                    mysql_free_result(result);
                    return accountId;
                }
            }
            mysql_free_result(result);
        }
    }
    else
    {
        cerr << " [DBManager] VerifyAccount Query Error: " << mysql_error(_conn) << endl;
    }
    return -1;
}

shared_ptr<Player> DBManager::LoadPlayerProfile(int32_t accountId) {
    std::lock_guard<std::mutex> lock(_dbLock);

    if (!_conn) return nullptr;

    string query = std::format("SELECT profile_id,nickname, gold, player_level FROM player_profile WHERE account_id = {}", accountId);
    
    if (mysql_query(_conn, query.c_str()) == 0)
    {
        MYSQL_RES* result = mysql_store_result(_conn);
        if (result)
        {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row != nullptr)
            {
                int32_t profileId = std::stoi(row[0]);
                string nickname = row[1] ? row[1] : "";
                int32_t gold = std::stoi(row[2]);
                int32_t level = std::stoi(row[3]);

                mysql_free_result(result);
                return make_shared<Player>(profileId, nickname, gold, level);
            }
        }
        mysql_free_result(result);
    }
    return nullptr;
}

bool DBManager::CreatePlayerProfile(int32_t accountId, const string& nickname)
{
    std::lock_guard<std::mutex> lock(_dbLock);
    if (!_conn) return false;

    string query = std::format("INSERT INTO player_profile(account_id, nickname, gold, player_level) VALUES({}, '{}', 1000, 1)",
        accountId, nickname);

    if (mysql_query(_conn, query.c_str()) == 0)
    {
        return true;
    }
    else
    {
        cerr << " [DBManager] CreateProfile Query Error: " << mysql_error(_conn) << endl;
        return false;
    }

    return false;
}
