#include "pch.h"
#include "DBManager.h"


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
