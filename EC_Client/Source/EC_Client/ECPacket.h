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

	RefreshShopReq = 7,
	RefreshShopRes = 8,
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
	uint32 AccountId;
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
	FPacketHeader Header;
};

struct FPKT_S2C_EnterGameRes
{
	FPacketHeader Header;
	bool bSuccess;

	uint32 gold;
	uint32 level;
	uint32 currentWave;
};

/*=================
	리롤
=================*/

struct FPKT_C2S_RefreshShopReq
{
	FPacketHeader Header;
};

// [서버 -> 클라] 상점 갱신 결과 전달
struct FPKT_S2C_RefreshShopRes
{
	FPacketHeader Header;
	bool bSuccess;
	uint32 remainGold;
	uint32 shopUnits[5];
};

#pragma pack(pop)