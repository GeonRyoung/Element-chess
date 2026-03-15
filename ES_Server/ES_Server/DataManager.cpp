#include "pch.h"
#include "DataManager.h"
#include "DBManager.h"

void DataManager::Init()
{
	try
	{
		mysqlx::Schema db = GDBManager->GetSchema();
		mysqlx::Table unitTable = db.getTable("unit_information");

		auto result = unitTable.select("unit_id", "name", "type", "role", "unit_stats").execute();

		for (mysqlx::Row row : result.fetchAll())
		{
			UnitMasterData data;
			data.unitId = static_cast<int32_t>(row[0]);
			data.name = static_cast<string>(row[1]);
			data.type = static_cast<string>(row[2]);
			data.role = static_cast<string>(row[3]);
			data.statsJson = static_cast<string>(row[4]);	
			_unitDatas[data.unitId] = data;
		}

		cout << " [DataManger] " << _unitDatas.size() << " Units Cached." << "\n";
	}
	catch (const std::exception& e)
	{
		cerr << " [DataMange] Error:" << e.what() << "\n";
	}
}

UnitMasterData* DataManager::GetUnitData(int32_t unitId)
{
	if (_unitDatas.find(unitId) != _unitDatas.end())
		return &_unitDatas[unitId];
	return nullptr;
}
