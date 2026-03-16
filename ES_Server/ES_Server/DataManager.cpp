#include "pch.h"
#include "DataManager.h"
#include "DBManager.h"

void DataManager::Init()
{
	MYSQL* conn = GDBManager->GetConnection();
	if (!conn)
	{
		cerr << " [DataManager] DB Connection is null!" << "\n";
		return;
	}

	const char* query = "SELECT unit_id, name, type, role, unit_stats FROM unit_information";

	if (mysql_query(conn, query) == 0)
	{
		MYSQL_RES* result = mysql_store_result(conn);
		if (result)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(result)))
			{
				UnitMasterData data;
				data.unitId = std::stoi(row[0]);
				data.name = row[1] ? row[1] : "";
				data.type = row[2] ? row[2] : "";
				data.role = row[3] ? row[3] : "";
				data.statsJson = row[4] ? row[4] : "";

				_unitDatas[data.unitId] = data;
			}
			mysql_free_result(result);
			cout << " [DataManger] " << _unitDatas.size() << " Units Cached." << "\n";
		}
	}
	else
	{
		cerr << " [DataMange] Query Error: " << mysql_error(conn) << "\n";
	}
}

UnitMasterData* DataManager::GetUnitData(int32_t unitId)
{
	if (_unitDatas.find(unitId) != _unitDatas.end())
		return &_unitDatas[unitId];
	return nullptr;
}
