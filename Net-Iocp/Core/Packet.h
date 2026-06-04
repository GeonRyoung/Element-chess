#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <cassert>

// =============================================================
//  PacketOpcode — 패킷 종류 식별자
//  상위 바이트 = 카테고리, 하위 바이트 = 세부 번호
//  0x00xx : 연결/인증   0x01xx : 이동
//  0x02xx : 전투        0x03xx : 스폰/소멸
//  0x04xx : 종료        0x0Fxx : RUDP 제어
// =============================================================
enum class PacketOpcode : uint16_t
{
    // ── 연결 / 인증 ──────────────────────────────────────────
    CS_CONNECT          = 0x0001,   // C→S  접속 요청
    SC_CONNECT_ACK      = 0x0002,   // S→C  접속 승인 + sessionId 발급
    CS_AUTH             = 0x0003,   // C→S  인증 토큰 전송
    SC_AUTH_ACK         = 0x0004,   // S→C  인증 결과

    // ── 이동 / 위치 ──────────────────────────────────────────
    CS_POSITION_UPDATE  = 0x0101,   // C→S  내 위치 업데이트
    SC_POSITION_UPDATE  = 0x0102,   // S→C  다른 액터 위치 브로드캐스트

    // ── 전투 ─────────────────────────────────────────────────
    CS_COMBAT_ACTION    = 0x0201,   // C→S  공격/스킬 사용
    SC_COMBAT_RESULT    = 0x0202,   // S→C  전투 판정 결과 브로드캐스트

    // ── 스폰 / 디스폰 ────────────────────────────────────────
    SC_SPAWN            = 0x0301,   // S→C  시야에 새 액터 등장
    SC_DESPAWN          = 0x0302,   // S→C  시야에서 액터 사라짐

    // ── 연결 종료 ────────────────────────────────────────────
    CS_DISCONNECT       = 0x0401,   // C→S  정상 종료 요청
    SC_DISCONNECT       = 0x0402,   // S→C  강제 종료 통보

    // ── RUDP 제어 ────────────────────────────────────────────
    RUDP_ACK            = 0x0F01,   // 양방향  수신 확인 + Selective Repeat 비트맵

    INVALID             = 0xFFFF,
};

// =============================================================
//  PacketHeader — 모든 패킷의 고정 앞부분 
// =============================================================
#pragma pack(push, 1)

struct PacketHeader
{
    uint16_t size;      // 헤더 포함 전체 패킷 크기 
    uint16_t opcode;    // PacketOpcode 값
    uint32_t sequence;  // RUDP 시퀀스 번호 
};

static_assert(sizeof(PacketHeader) == 8, "PacketHeader must be exactly 8 bytes");

// =============================================================
//  패킷 바디 구조체 정의
// =============================================================

// ── 연결 / 인증 ──────────────────────────────────────────────

struct SC_ConnectAckBody        // S→C  접속 승인
{
    uint64_t sessionId;         // 서버가 발급한 세션 고유 ID
    uint8_t  result;            // 0=OK  1=서버 만원  2=접속 불가
};

struct CS_AuthBody              // C→S  인증 요청
{
    char token[64];             // 인증 토큰 
};

struct SC_AuthAckBody           // S→C  인증 결과
{
    uint8_t result;             // 0=OK  1=토큰 불일치  2=타임아웃
};

// ── 이동 / 위치 ──────────────────────────────────────────────

struct CS_PositionBody          // C→S  내 위치 전송
{
    float x;
    float y;
};

struct SC_PositionBody          // S→C  다른 액터 위치 브로드캐스트
{
    uint64_t sessionId;         // 이동한 액터의 sessionId
    float    x;
    float    y;
};

// ── 전투 ─────────────────────────────────────────────────────

struct CS_CombatActionBody      // C→S  전투 행동 요청
{
    uint64_t targetSessionId;   // 공격 대상 sessionId
    uint16_t skillId;           // 사용한 스킬 ID
};

struct SC_CombatResultBody      // S→C  전투 결과 브로드캐스트
{
    uint64_t attackerSessionId;
    uint64_t targetSessionId;
    int32_t  damage;            // 양수=피해  음수=회복
    uint8_t  isCritical;        // 0=일반  1=크리티컬
};

// ── 스폰 / 디스폰 ────────────────────────────────────────────

struct SC_SpawnBody             // S→C  새 액터 시야 진입
{
    uint64_t sessionId;
    float    x;
    float    y;
};

struct SC_DespawnBody           // S→C  액터 시야 이탈
{
    uint64_t sessionId;
};

// ── RUDP ACK ─────────────────────────────────────────────────

struct RudpAckBody
{
    uint32_t ackNumber;         // 정상 수신한 마지막 시퀀스 번호
    uint32_t ackBitmap;         // Selective Repeat 비트맵 (이전 32개 수신 여부)
};

#pragma pack(pop)

// =============================================================
//  Packet — 헤더 + 페이로드 직렬화/역직렬화 유틸리티
// =============================================================
static constexpr uint16_t MAX_PACKET_SIZE = 512;

class Packet
{
private:
    char     m_buffer[MAX_PACKET_SIZE];
    uint16_t m_writeOffset;     // 다음 Write() 가 쓸 위치

public:
    Packet() : m_writeOffset(sizeof(PacketHeader))
    {
        std::memset(m_buffer, 0, sizeof(m_buffer));
    }

    // 패킷 초기화 — 보내기 직전에 반드시 호출
    void Init(PacketOpcode opcode, uint32_t sequence = 0)
    {
        m_writeOffset       = sizeof(PacketHeader);
        PacketHeader* hdr   = GetHeader();
        hdr->opcode         = static_cast<uint16_t>(opcode);
        hdr->sequence       = sequence;
        hdr->size           = sizeof(PacketHeader);
    }

    // 바디 구조체를 버퍼에 직렬화 (송신 측)
    template<typename T>
    bool Write(const T& body)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "Packet body must be trivially copyable");

        if (m_writeOffset + sizeof(T) > MAX_PACKET_SIZE)
            return false;

        std::memcpy(m_buffer + m_writeOffset, &body, sizeof(T));
        m_writeOffset           += static_cast<uint16_t>(sizeof(T));
        GetHeader()->size       = m_writeOffset;
        return true;
    }

    // 버퍼에서 바디 구조체를 역직렬화 (수신 측)
    template<typename T>
    bool Read(T& out, uint16_t readOffset = sizeof(PacketHeader)) const
    {
        if (readOffset + sizeof(T) > GetHeader()->size)
            return false;

        std::memcpy(&out, m_buffer + readOffset, sizeof(T));
        return true;
    }

    // CircularBuffer/WSASend 에 넘겨줄 원시 버퍼
    char*             GetBuffer()       { return m_buffer; }
    const char*       GetBuffer() const { return m_buffer; }

    PacketHeader* GetHeader()
    {
        return reinterpret_cast<PacketHeader*>(m_buffer);
    }
    const PacketHeader* GetHeader() const
    {
        return reinterpret_cast<const PacketHeader*>(m_buffer);
    }

    uint16_t GetTotalSize() const
    {
        return GetHeader()->size;
    }
    PacketOpcode GetOpcode() const
    {
        return static_cast<PacketOpcode>(GetHeader()->opcode);
    }
};

using PacketPtr = std::shared_ptr<Packet>;

