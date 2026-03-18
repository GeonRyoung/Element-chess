#pragma once
#include <cstdint>

enum class EPacketId : uint16_t
{
	LoginReq = 1,
	LoginRes = 2
};

#pragma pack(push, 1)

struct PacketHeader
{
	uint16_t size;
	uint16_t id;
};

struct PKT_C2S_LoginReq
{
	PacketHeader header;
	char username[32];
	char password[32]; 
};

struct PKT_S2C_LoginRes
{
	PacketHeader header;
	bool bSuccess;
	int32_t accountId;

	bool bHasProfile;
	char nickname[32];
};

#pragma pack(pop)^