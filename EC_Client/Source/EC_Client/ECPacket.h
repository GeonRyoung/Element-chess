#pragma once

#include "CoreMinimal.h"

enum class EPacketID : uint16
{
	LoginReq = 1,
	LoginRes = 2,

	CreateNicknameReq = 3,
	CreateNicknameRes = 4,

	EnterGameReq = 5,
	EnterGameRes = 6,
};

#pragma pack(push, 1)

struct FPacketHeader
{
	uint16 Size;
	uint16 ID;
};

/*=================
	로그인
=================*/

struct FPKT_C2S_LoginReq
{
	FPacketHeader Header;
	char Username[32];
	char Password[32];
};

struct FPKT_S2C_LoginRes
{
	FPacketHeader Header;
	bool bSuccess;
	int32 AccountId;
	bool bHasProfile;
	char Nickname[32];
};

/*=================
	닉네임 생성
=================*/

struct FPKT_C2S_CreateNicknameReq
{
	FPacketHeader Header;
	char Nickname[32];
};

struct FPKT_S2C_CreateNicknameRes
{
	FPacketHeader Header;
	bool bSuccess;
};

/*=================
	게임 시작
=================*/

struct FPKT_C2S_EnterGameReq
{
	FPacketHeader header;
};

struct FPKT_S2C_EnterGameRes
{
	FPacketHeader header;
	bool bSuccess;

	int32_t gold;
	int32_t level;
	int32_t currentWave;
	int32_t playerHp;

	int32_t maxBattleSlots;
	int32_t maxBenchSlots;
};

#pragma pack(pop)