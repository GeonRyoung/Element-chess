#pragma once

#include "CoreMinimal.h"

enum class EPacketID : uint16
{
	LoginReq = 1,
	LoginRes = 2,
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
};

#pragma pack(pop)