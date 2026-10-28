#pragma once
#include <cstdint>

// =====================================================================
// Protocol.h : 클라이언트와 서버가 같이 쓰는 패킷 정의
// 모든 패킷은 PacketHeader(3바이트)로 시작한다.
// =====================================================================

#pragma pack(push, 1)

// ===== 상수 =====
constexpr uint8_t MAX_PLAYERS = 4;              // 최대 접속 인원
constexpr uint8_t MIN_PLAYERS_TO_START = 2;     // 게임을 시작할 수 있는 최소 인원
constexpr uint8_t MAX_ENEMIES = 32;             // 한 맵의 AI 적 최대 수
constexpr uint8_t MAX_BULLETS = 64;             // 동시에 날아다니는 총알 최대 수
constexpr uint8_t INVALID_PLAYER_ID = 255;      // "없음" (거절됨, 승자 없음)
constexpr int TICK_RATE = 30;                   // 서버 틱, 클라이언트 프레임 (회/초)
constexpr uint16_t SERVER_TCP_PORT = 9000;      // 서버 TCP 대기 포트 (예시 값)
constexpr uint16_t SERVER_UDP_PORT = 9001;      // 서버 UDP 입력 받기 포트 (예시 값)

// ===== enum =====
enum class EPacketType : uint8_t
{
    // 클라이언트 -> 서버
    CS_Ready = 1,
    CS_Input,

    // 서버 -> 클라이언트
    SC_ConnectResult = 100,
    SC_LobbyState,
    SC_GameStart,
    SC_PlayerStates,
    SC_EnemyStates,
    SC_BulletStates,
    SC_Event,
    SC_GameResult
};

enum class EPlayerType : uint8_t { None = 0, Helicopter, Tank };            // 기체 종류
enum class EPlayerState : uint8_t { Alive = 0, Ghost };                     // Ghost = 사망 후 관전 (이동만 가능)
enum class EEventType : uint8_t { PlayerDied = 0, EnemyDied };              // SC_Event 종류
enum class ERejectReason : uint8_t { None = 0, ServerFull, AlreadyStarted }; // 접속 거절 이유

// ===== 공통 헤더 =====
struct PacketHeader
{
    uint16_t size;          // 헤더를 포함한 패킷 전체 바이트 수
    EPacketType type;       // 패킷 종류
};

// ===== 클라이언트 -> 서버 =====
struct CS_Ready             // TCP. 로비에서 대기 버튼을 처음 누를 때 1회
{
    PacketHeader header;    // 내용 없음 (헤더만)
};

struct CS_Input             // UDP. 매 프레임 (30회/초)
{
    PacketHeader header;
    uint8_t playerId;           // 보낸 플레이어 (UDP는 연결이 없어서 패킷에 적음)
    uint32_t sequence;          // 입력 번호. 매 프레임 +1, 게임이 바뀌어도 되돌리지 않음
    int8_t moveForward;         // -1(S), 0, 1(W)
    int8_t moveRight;           // -1(A), 0, 1(D)
    float cameraYaw;            // 카메라 yaw (도). 이동 방향의 기준
    uint8_t fire;               // 좌클릭을 누르고 있으면 1
    uint8_t shieldPressCount;   // E를 누른 횟수. 누를 때마다 +1, 서버는 값이 바뀌면 쉴드 토글
};

// ===== 서버 -> 클라이언트 =====
struct SC_ConnectResult     // TCP. accept 직후 그 클라이언트에게 1회
{
    PacketHeader header;
    uint8_t accepted;           // 1 = 승인, 0 = 거절
    uint8_t playerId;           // 내 PlayerId (거절이면 INVALID_PLAYER_ID)
    uint8_t rejectReason;       // ERejectReason
};

struct LobbyPlayerInfo
{
    uint8_t playerId;
    uint8_t ready;              // 대기 버튼을 눌렀으면 1
};

struct SC_LobbyState        // TCP. 접속·준비·끊김·로비 복귀 때 전원에게
{
    PacketHeader header;
    uint8_t playerCount;                        // 접속 인원 (앞에서 playerCount칸이 유효)
    LobbyPlayerInfo players[MAX_PLAYERS];       // 항상 4칸 모두 보냄
};

struct PlayerStartInfo
{
    uint8_t playerId;
    uint8_t playerType;         // EPlayerType
    float x, y, z;              // 시작 위치
};

struct SC_GameStart         // TCP. 게임 준비 때 전원에게 1회
{
    PacketHeader header;
    uint8_t mapNumber;                          // 1 ~ 3
    uint8_t playerCount;                        // 이번 게임 참가 인원
    PlayerStartInfo players[MAX_PLAYERS];       // 항상 4칸 모두 보냄
};

struct PlayerStateInfo
{
    uint8_t playerId;
    uint8_t state;              // EPlayerState (Alive / Ghost)
    uint8_t shieldEnabled;      // 쉴드가 켜져 있으면 1
    uint8_t health;             // 0 ~ 100
    float x, y, z;              // 위치
    float yaw;                  // 기체가 바라보는 방향 (도)
};

struct SC_PlayerStates      // UDP. 매 틱, 클라마다 sendto
{
    PacketHeader header;
    uint32_t tick;                              // 틱 번호. 게임이 바뀌어도 되돌리지 않음
    uint8_t count;                              // 들어 있는 플레이어 수 (나 포함, 최대 4)
    PlayerStateInfo players[MAX_PLAYERS];       // 플레이어마다 1칸. count칸만 보냄
};

struct EnemyStateInfo
{
    uint8_t enemyIndex;         // 게임 시작 때 맵 Spawn 순서 (서버와 클라가 같음, 종류도 이 번호로 앎)
    float x, y, z;              // 위치
    float yaw;                  // 바라보는 방향 (도)
};

struct SC_EnemyStates       // UDP. 매 틱, 클라마다 sendto
{
    PacketHeader header;
    uint32_t tick;
    uint8_t count;                              // 살아 있는 적 수
    EnemyStateInfo enemies[MAX_ENEMIES];        // 살아 있는 적만 count칸 보냄 (죽음은 SC_Event로 알림)
};

struct BulletStateInfo
{
    float x, y, z;              // 총알 위치
};

struct SC_BulletStates      // UDP. 매 틱, 클라마다 sendto
{
    PacketHeader header;
    uint32_t tick;
    uint8_t count;                              // 날아가는 총알 수
    BulletStateInfo bullets[MAX_BULLETS];       // count칸만 보냄
};

struct SC_Event             // TCP. 사망이 일어난 틱에 전원에게
{
    PacketHeader header;
    uint8_t eventType;          // EEventType
    uint8_t subjectId;          // PlayerDied: 죽은 PlayerId / EnemyDied: enemyIndex
    float x, y, z;              // 사망 위치 (폭발 연출용)
};

struct SC_GameResult        // TCP. 생존자가 1명 이하가 된 틱에 전원에게 1회
{
    PacketHeader header;
    uint8_t winnerPlayerId;     // 승자. 동시 사망으로 승자가 없으면 INVALID_PLAYER_ID (전원 패배)
};

#pragma pack(pop)
