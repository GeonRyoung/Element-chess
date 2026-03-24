#pragma once
#include <cstdint>

enum class EPacketId : uint16_t
{
	LoginReq = 1,
	LoginRes = 2,

	CreateNicknameReq = 3,
	CreateNicknameRes = 4,

	EnterGameReq = 5,
	EnterGameRes = 6,
};

#pragma pack(push, 1)

struct PacketHeader
{
	uint16_t size;
	uint16_t id;
};

/*=================
	로그인
=================*/

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

/*=================
	닉네임 생성
=================*/

struct PKT_C2S_CreateNicknameReq
{
	PacketHeader header;
	char nickname[32];
};

struct PKT_S2C_CreateNicknameRes
{
	PacketHeader header;
	bool bSuccess;
};

/*=================
	게임 시작
=================*/

struct PKT_C2S_EnterGameReq
{
	PacketHeader header;
};

struct PKT_S2C_EnterGameRes
{
	PacketHeader header;
	bool bSuccess;
};

#pragma pack(pop)