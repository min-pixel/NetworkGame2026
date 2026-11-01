// 클라이언트와 서버가 함께 쓰는 패킷 정의 (패킷 10종, enum, 상수)
// 규칙
// - 헤더 3바이트: uint16_t size(헤더 포함 전체 크기) + uint8_t type
// - #pragma pack(push, 1)로 빈틈 없이 묶는다. 자료형은 uint8_t, int8_t, uint16_t, uint32_t, float만 쓴다
// - 바이트 순서: IP와 포트만 htons 등으로 바꾸고, 게임 데이터는 그대로 보낸다 (전부 x64 Windows)
// - 상태 3종(SC_PlayerStates, SC_EnemyStates, SC_BulletStates)은 앞 8바이트(헤더 3 + tick 4 + count 1) + count칸만 보낸다
// - EPlayerType은 베이스 게임의 PlayerType.h 것을 쓴다. 여기에 다시 정의하지 않는다 (패킷에는 uint8_t로 바꿔 넣음)

#pragma once

#include <cstdint>

// 상수
constexpr int MAX_PLAYERS = 4;
constexpr int MIN_PLAYERS_TO_START = 2;
constexpr int MAX_ENEMIES = 32;
constexpr int MAX_BULLETS = 64;
constexpr uint8_t INVALID_PLAYER_ID = 255;
constexpr int TICK_RATE = 30;
constexpr uint16_t SERVER_TCP_PORT = 9000;
constexpr uint16_t SERVER_UDP_PORT = 9001;

// 패킷 종류 (CS = 클라 -> 서버, SC = 서버 -> 클라)
enum class EPacketType : uint8_t
{
    CS_Ready = 1,       // TCP
    CS_Input,           // UDP

    SC_ConnectResult = 100, // TCP
    SC_LobbyState,      // TCP
    SC_GameStart,       // TCP
    SC_PlayerStates,    // UDP
    SC_EnemyStates,     // UDP
    SC_BulletStates,    // UDP
    SC_Event,           // TCP
    SC_GameResult       // TCP
};

// 플레이어 상태 (PlayerStateInfo::state)
enum class EPlayerState : uint8_t
{
    Alive = 0,
    Ghost
};

// 이벤트 종류 (SC_Event::eventType)
enum class EEventType : uint8_t
{
    PlayerDied = 0,
    EnemyDied
};

// 접속 거절 이유 (SC_ConnectResult::rejectReason)
enum class ERejectReason : uint8_t
{
    None = 0,
    ServerFull,
    AlreadyStarted
};

#pragma pack(push, 1)

// 모든 패킷 앞 3바이트
struct PacketHeader
{
    uint16_t size;  // 헤더 포함 전체 크기
    uint8_t type;   // EPacketType 값
};

// 배열 한 칸 구조체

// 로비 화면 한 줄 (2B)
struct LobbyPlayerInfo
{
    uint8_t playerId;
    uint8_t ready;      // 0 또는 1
};

// 게임 시작 때 한 사람 정보 (14B)
struct PlayerStartInfo
{
    uint8_t playerId;
    uint8_t playerType; // EPlayerType 값 (PlayerType.h)
    float x, y, z;
};

// 플레이어 상태 한 칸 (20B)
struct PlayerStateInfo
{
    uint8_t playerId;
    uint8_t state;          // EPlayerState 값
    uint8_t shieldEnabled;  // 0 또는 1
    uint8_t health;
    float x, y, z;
    float yaw;
};

// 적 상태 한 칸 (17B)
struct EnemyStateInfo
{
    uint8_t enemyIndex;
    float x, y, z;
    float yaw;
};

// 총알 상태 한 칸 (12B). ID 없이 활성 총알을 앞에서부터 count개 보내고, 클라는 표시 칸에 순서대로 넣는다
struct BulletStateInfo
{
    float x, y, z;
};

// 패킷 10종

// TCP, 클라 -> 서버: 로비에서 Ready 버튼 (3B)
struct CS_Ready
{
    PacketHeader header;
};

// UDP, 클라 -> 서버: 매 프레임 키 입력 (16B)
struct CS_Input
{
    PacketHeader header;
    uint8_t playerId;
    uint32_t sequence;          // 보낼 때마다 1씩 증가, 새 게임에서도 되돌리지 않음
    int8_t moveForward;         // -1, 0, 1 (W/S)
    int8_t moveRight;           // -1, 0, 1 (D/A)
    float cameraYaw;            // 카메라 좌우 각도(도). 서버가 이 값으로 앞, 오른쪽 방향을 만든다
    uint8_t fire;               // 0 또는 1 (좌클릭 누르고 있음)
    uint8_t shieldPressCount;   // E를 누른 누적 횟수. 서버는 값이 바뀌면 쉴드를 켜고 끈다
};

// TCP, 서버 -> 클라: 접속하면 서버가 바로 보냄 (6B)
struct SC_ConnectResult
{
    PacketHeader header;
    uint8_t accepted;       // 0 또는 1
    uint8_t playerId;       // 거절이면 INVALID_PLAYER_ID
    uint8_t rejectReason;   // ERejectReason 값
};

// TCP, 서버 -> 전원: 로비 인원과 Ready 상태 (12B)
struct SC_LobbyState
{
    PacketHeader header;
    uint8_t playerCount;
    LobbyPlayerInfo players[MAX_PLAYERS];
};

// TCP, 서버 -> 전원: 게임 시작 (61B)
struct SC_GameStart
{
    PacketHeader header;
    uint8_t mapNumber;      // 1 ~ 3
    uint8_t playerCount;
    PlayerStartInfo players[MAX_PLAYERS];
};

// UDP, 서버 -> 클라마다: 플레이어 상태 (8 + 20 x count, 최대 88B)
struct SC_PlayerStates
{
    PacketHeader header;
    uint32_t tick;
    uint8_t count;
    PlayerStateInfo players[MAX_PLAYERS];
};

// UDP, 서버 -> 클라마다: 적 상태 (8 + 17 x count, 최대 552B)
struct SC_EnemyStates
{
    PacketHeader header;
    uint32_t tick;
    uint8_t count;
    EnemyStateInfo enemies[MAX_ENEMIES];
};

// UDP, 서버 -> 클라마다: 총알 상태 (8 + 12 x count, 최대 776B)
struct SC_BulletStates
{
    PacketHeader header;
    uint32_t tick;
    uint8_t count;
    BulletStateInfo bullets[MAX_BULLETS];
};

// TCP, 서버 -> 전원: 사망 이벤트 (17B). 죽인 사람은 보내지 않는다
struct SC_Event
{
    PacketHeader header;
    uint8_t eventType;  // EEventType 값
    uint8_t subjectId;  // PlayerDied면 playerId, EnemyDied면 enemyIndex
    float x, y, z;      // 죽은 위치 (폭발 연출용)
};

// TCP, 서버 -> 전원: 게임 결과 (4B)
struct SC_GameResult
{
    PacketHeader header;
    uint8_t winnerPlayerId; // 승자가 없으면(동시 사망) INVALID_PLAYER_ID
};

#pragma pack(pop)

// 상태 3종에서 배열 앞까지의 크기 (헤더 3 + tick 4 + count 1)
constexpr int STATE_PACKET_HEAD_SIZE = 8;
