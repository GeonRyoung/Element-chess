#include "pch.h"
#include "DBManager.h"

/*=====================
    DB 연결
=====================*/

bool DBManager::Connect(const string& host, int port, const string& user, const string& password, const string& schema,
    int32_t poolSize)
{
    for (int i = 0; i < poolSize; i++)
    {
        MYSQL* conn = mysql_init(nullptr);
        if (conn == nullptr) return false;
        
        if (mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
            schema.c_str(), port, nullptr, 0 ) == 0)
        {
            mysql_close(conn);
            return false;
        }
        
        mysql_set_character_set(conn, "utf8mb4");
        
        _connectionPool.push(conn);
    }
    cout << " [DBManager] Successfully connected to database: " << schema << " (Pool Size: " << poolSize << ")" << endl;
    return true;
}

void DBManager::Disconnect()
{
    std::lock_guard<std::mutex> lock(_poolLock);
    while (!_connectionPool.empty())
    {
        MYSQL* conn = _connectionPool.front();
        _connectionPool.pop();
        if (conn) mysql_close(conn);
    }
    cout << " [DBManager] Disconnected from database." << endl;
}



/*=====================
    커넥션 대여 및 반납
=====================*/

MYSQL* DBManager::PopConnection()
{
    std::unique_lock<std::mutex> lock(_poolLock);
    
    _poolCv.wait(lock, [this]() { return !_connectionPool.empty(); });

    MYSQL* conn = _connectionPool.front();
    _connectionPool.pop();
    return conn;
}

void DBManager::PushConnection(MYSQL* conn)
{
    if (conn == nullptr) return;

    std::unique_lock<std::mutex> lock(_poolLock);
    _connectionPool.push(conn);
    
    _poolCv.notify_one();
}

/*=====================
    로그인
=====================*/



int32_t DBManager::VerifyAccount(const string& username, const string& passwordHash) {
    DBConnectionGuard guard;
    MYSQL* conn = guard.Get();

    string query = std::format("SELECT account_id, password_hash FROM user_account WHERE username = '{}'", username);

    if (mysql_query(conn, query.c_str()) == 0)
    {
        MYSQL_RES* result = mysql_store_result(conn);
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
        cerr << " [DBManager] VerifyAccount Query Error: " << mysql_error(conn) << endl;
    }
    return -1;
}

shared_ptr<Player> DBManager::LoadPlayerProfile(int32_t accountId) {
    DBConnectionGuard guard;
    MYSQL* conn = guard.Get();

    string query = std::format("SELECT profile_id,nickname, gold, player_level FROM player_profile WHERE account_id = {}", accountId);
    
    if (mysql_query(conn, query.c_str()) == 0)
    {
        MYSQL_RES* result = mysql_store_result(conn);
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
    DBConnectionGuard guard;
    MYSQL* conn = guard.Get();

    string query = std::format("INSERT INTO player_profile(account_id, nickname, gold, player_level) VALUES({}, '{}', 1000, 1)",
        accountId, nickname);

    if (mysql_query(conn, query.c_str()) == 0)
    {
        return true;
    }
    else
    {
        cerr << " [DBManager] CreateProfile Query Error: " << mysql_error(conn) << endl;
        return false;
    }
}
