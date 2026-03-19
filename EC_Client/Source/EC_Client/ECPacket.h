#pragma once

#include "CoreMinimal.h"

enum class EPacketID : uint16
{
	LoginReq = 1,
	LoginRes = 2,

	CreateNicknameReq = 3,
	CreateNicknameRes = 4
};

#pragma pack(push, 1)

struct FPacketHeader
{
	uint16 Size;
	uint16 ID;
};

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

#pragma pack(pop)