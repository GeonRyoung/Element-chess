#pragma once

struct UnitMasterData
{
	int32_t unitId;
	string name;
	string type;
	string role;
	string statsJson;
};

class DataManager
{
public:
	static DataManager* GetInstance()
	{
		static DataManager instance;
		return &instance;
	}

	void Init();
	UnitMasterData* GetUnitData(int32_t unitId);

private:
	DataManager() = default;
	unordered_map<int32_t, UnitMasterData> _unitDatas;
};


