// 로우레벨.h
// 로우 레벨 디자인 — DirectX12 TPS 4인 개인전 (서버 권한형, Winsock)
//
// 이 파일은 무엇인가
//   로우 레벨 디자인 문서의 3~5장(클래스·구조체·함수)을 헤더 파일 모양으로 한곳에 모은 것이다.
//   설계를 읽기 위한 정리본이라 이대로 컴파일하지는 않는다. 실제 코드에서는 클래스마다 적은
//   파일 이름(GameServer.h 등)으로 나눠 만든다.
//
// 읽는 법
//   - 함수 선언 한 줄 = 리턴 타입 + 함수명 + (인자). 바로 위 주석이 그 함수가 하는 일이다.
//   - '인자 ○○:' 줄은 그 인자에 무엇을 넘기는지다. 이름이 out으로 시작하는 인자는
//     함수가 결과를 그 변수에 채워서 돌려준다는 뜻이다.
//   - '===== ○○ =====' 줄은 그 아래 함수를 어느 스레드(동시에 도는 작업 줄기)가 실행하는지다.
//   - [새] = 이번에 새로 만드는 것.
//     [기존] = 베이스 게임에 이미 있는 것을 그대로 쓴다.
//     [기존, 바뀜] = 베이스 게임에 있던 함수에 하는 일이 더해진다.
//   - 베이스 게임 클래스는 더하거나 바뀌는 것만 적었다.
//   - (문서 3-1)처럼 적은 번호는 로우 레벨 디자인 문서의 절 번호다.
//   - 패킷 구조체(CS_로 시작 = 클라 → 서버, SC_로 시작 = 서버 → 클라)와 상수(MAX_PLAYERS = 4 등)는
//     Protocol.h에 있다.
//
// 자주 나오는 말
//   스레드      한 프로그램 안에서 동시에 도는 작업 줄기
//   잠금        여러 스레드가 같이 쓰는 데이터를 한 번에 한 스레드만 만지게 하는 것(CRITICAL_SECTION, 임계 영역)
//   이벤트      기다리는 스레드를 깨우는 신호(SetEvent로 깨우고 WaitForSingleObject로 기다린다)
//   큐          받은 TCP 패킷을 도착 순서대로 쌓아 두는 목록
//   자리        서버가 접속한 클라를 적어 두는 칸. 4칸이고 칸 번호(0~3)가 PlayerId
//   틱 / 프레임 서버가 게임을 한 번 진행하는 단위 / 클라가 화면을 한 번 그리는 단위. 둘 다 1/30초마다
//   유령        죽은 플레이어 상태. 움직일 수만 있고 쏘거나 맞지 않는다
//   255         INVALID_PLAYER_ID. '없음'(거절됨, 승자 없음)
//
// 목록 (들여쓴 것은 바로 위 클래스가 멤버로 가지고 있는 것)
//
//   [공통]  NetUtil.h
//     SendAll, RecvPacket              TCP로 끝까지 보내기 / TCP 패킷 하나 받기
//     NetEvent                         TCP로 받은 패킷 하나 (큐의 한 칸)
//     LatestStates                     UDP로 받은 최신 상태 3종
//     PlayerInput (기존)               한 프레임의 입력
//     GameSessionConfig (기존 + 더함)  클라가 게임 화면을 만들 때 넘기는 설정
//
//   [서버]
//     CGameServer          GameServer.h    소켓 준비, 접속 받기, 자리 등록, 준비·시작 확인, TCP 보내기, UDP 입력 받기
//       CClientSession     구조체           자리 한 칸 (4칸)
//       InputSlot          구조체           입력 한 칸 (4칸)
//       CGameLoop          GameLoop.h      게임 한 판 준비 → 1/30초마다 진행 → 결과 → 로비로 되돌리기
//         CGameTimer         (기존)        1/30초 간격 맞추기
//         CTerrainGameMap    (기존)        맵 (지형 높이, 적이 나타나는 위치)
//         CGameWorld         (기존)        기체·적·총알과 이동·적 AI·피격 계산
//           CPlayer, CBullet (기존)        기체, 총알
//
//   [클라]
//     CGameFramework       (기존)          매 프레임 진행, 화면 바꾸기, 받은 패킷을 화면에 나눠 주기
//       CGameTimer         (기존)          1/30초 간격 맞추기
//       CNetworkClient     NetworkClient.h 접속, 패킷 보내기, TCP·UDP 받기, 큐와 최신 상태 보관
//       CSceneBase         (기존)          화면 4개의 부모 클래스
//         CStartScene        (기존)        시작 화면. 제목을 클릭하면 접속
//         CMapSelectScene    (기존)        로비 화면. 준비 버튼과 자리 4칸
//         CGameScene         (기존)        게임 화면. 입력 보내기, 받은 상태 보여 주기
//         CEndScene          (기존)        결과 화면. 승리 또는 패배

#pragma once

#include <winsock2.h>
#include <windows.h>
#include <cstdint>
#include <vector>
#include "Protocol.h"

// 아래에서 서로 가리키는 클래스 (먼저 이름만 알려 둠)
class CGameServer;
class CGameLoop;
class CNetworkClient;
class CGameObject;
class CGameMap;
class CEnemy;
class CPlayer;

// ==========================================================================================
// 1. 공통 (클라·서버가 같이 씀) — 문서 5장
// ==========================================================================================


// ----- 함수 (문서 5-1) -----

// NetUtil.h (새 파일)

// [새] len바이트가 다 나갈 때까지 send()를 반복한다(send 한 번에 다 안 나갈 수
// 있어서). 오류가 나면 false. 서버에서는 SendPacket 안에서만 부른다. 단, 거절 결과는
// 자리가 없어서 SendAll로 바로 보낸다.
//   인자 sock: 보낼 TCP 소켓
//   인자 data: 보낼 데이터
//   인자 len: 보낼 바이트 수
bool SendAll(SOCKET sock, const char* data, int len);

// [새] TCP로 패킷 하나를 받는다. 연결이 끊겼거나(recv가 0) 오류이거나 size가 이상하면
// false. 받을 때는 recv(..., MSG_WAITALL)를 써서 요청한 바이트가 다 올 때까지
// 기다린다.
// 하는 순서:
//   ① 헤더 3바이트를 다 받는다
//   ② 헤더의 size가 3 이상 bufSize 이하인지 확인한다
//   ③ 나머지 size - 3바이트를 다 받는다.
//   인자 sock: 받을 TCP 소켓
//   인자 buf: 받은 패킷을 담을 공간
//   인자 bufSize: buf의 크기(바이트)
bool RecvPacket(SOCKET sock, char* buf, int bufSize);


// ----- 구조체 (문서 5-2) -----

// NetUtil.h (새 파일)
// [새] TCP로 받은 패킷 하나. TCP 수신 스레드가 만들어 큐에 넣고, 메인 스레드가 꺼낸다.
struct NetEvent
{
    // 패킷 종류(헤더의 type)
    EPacketType       type;
    // 받은 패킷 전체(헤더 포함)
    std::vector<char> data;
};

// NetUtil.h (새 파일)
// [새] UDP로 받은 최신 상태 3종. UDP 수신 스레드가 덮어쓰고, 메인 스레드가 복사해 간다.
struct LatestStates
{
    // 최신 플레이어 상태
    SC_PlayerStates players;
    // 최신 적 상태
    SC_EnemyStates  enemies;
    // 최신 총알 상태
    SC_BulletStates bullets;
    // 종류별 '새 값 있음' 표시. UDP 수신 스레드가 덮어쓸 때 켜고, CopyLatestStates가
    // 끈다
    bool            hasNewPlayers;
    bool            hasNewEnemies;
    bool            hasNewBullets;
};

// PlayerInput.h (기존 파일)
// [기존] 한 프레임의 입력. 클라는 키 입력을 모을 때, 서버는 World를 계산할 때 쓴다.
struct PlayerInput
{
    // [기존] 앞뒤 이동 -1 ~ 1 (S/W)
    float moveForward;
    // [기존] 좌우 이동 -1 ~ 1 (A/D)
    float moveRight;
    // [기존] 마우스 이동량. 클라 카메라에만 쓴다
    float lookYawDelta;
    float lookPitchDelta;
    // [기존] 좌클릭을 누르고 있으면 true
    bool  fire;
    // [기존] 이번에 E를 눌렀으면 true
    bool  shieldTogglePressed;
};

// GameSessionConfig.h (기존 파일)
// [기존 + 더함] 클라가 게임 화면을 만들 때 넘기는 설정이다.
struct GameSessionConfig
{
    // [기존] 맵 번호 1~3
    int             mapNumber;
    // [기존] 내 기체 종류
    EPlayerType     playerType;
    // [기존] 게임 규칙. FreeForAll(개인전)로 쓴다
    EGameMode       gameMode;
    // [새] 내 PlayerId
    uint8_t         myPlayerId;
    // [새] 이번 게임 참가 인원
    uint8_t         playerCount;
    // [새] 참가자별 번호·기체 종류·시작 위치
    PlayerStartInfo players[MAX_PLAYERS];
};

// PlayerType.h (기존 파일) — 기체 종류. 패킷의 playerType 값(0, 1, 2)과 순서가 같다.
// 게임 코드에 이미 있으므로 여기서 다시 만들지 않고 PlayerType.h를 include해서 쓴다.
// enum class EPlayerType { None, Helicopter, Tank };

// ==========================================================================================
// 2. 베이스 게임 클래스에 더하는 것 (서버·클라 같이 씀) — 문서 3-5, 4-8
// ==========================================================================================


// ----- CGameTimer (문서 4-8) -----

// Timer.h (기존 파일) — 더하거나 바뀌는 것만 적음
class CGameTimer
{
public:
    // [기존] 지난 호출부터 1/fLockFPS초(30이면 1/30초)가 될 때까지 기다린다. 클라
    // FrameAdvance와 서버 WaitNextTick이 같이 쓴다.
    //   인자 fLockFPS: 1초에 몇 번으로 맞출지. 30을 넣는다
    void Tick(float fLockFPS = 0.0f);
};

// ----- CTerrainGameMap (문서 3-5) -----

// TerrainGameMap.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 서버 게임 루프 스레드가 부른다.
class CTerrainGameMap
{
public:
    // [기존] 맵 번호에 맞는 지형 높이 파일을 읽는다. 성공하면 true.
    bool LoadMapData();

    // [기존] 적이 나타날 위치 목록과 시작 위치를 채운다.
    void LoadSpawnPositions();

    // [새] index번 플레이어의 시작 위치를 돌려준다. 맵마다 4곳이 있다.
    //   인자 index: 플레이어 순서 0~3
    XMFLOAT3 GetPlayerStartPosition(int index);
};

// ----- CGameWorld (문서 3-5) -----

// GameWorld.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 서버 게임 루프 스레드가 부른다.
class CGameWorld
{
public:
    // [기존] 기체를 만들어 땅 위에 놓고, 만든 기체를 돌려준다.
    //   인자 playerType: 기체 종류(헬기·탱크)
    //   인자 position: 놓을 위치
    //   인자 gameMap: 맵(땅 높이를 알기 위해)
    CPlayer* SpawnPlayer(EPlayerType playerType, const XMFLOAT3& position, const CGameMap* gameMap);

    // [기존] 맵에 적힌 순서대로 적을 만든다. 이 순서가 적 번호(enemyIndex)가 된다.
    //   인자 gameMap: 맵
    void SpawnEnemies(const CGameMap* gameMap);

    // [기존] 총알을 미리 만들어 두고 꺼서 보관한다. 쏠 때마다 꺼진 총알을 켜서 쓴다.
    //   인자 bulletCount: 미리 만들 총알 수(64)
    void BuildBulletPool(int bulletCount);

    // [새] 한 기체의 이동과 쉴드를 처리한다. 유령은 이동만 한다.
    //   인자 player: 움직일 기체
    //   인자 deltaTime: 진행할 시간(1/30초)
    //   인자 input: 이 기체의 입력
    //   인자 cameraYaw: 이 플레이어 카메라의 좌우 방향(도). 앞으로 가는 방향의 기준
    //   인자 gameMap: 맵
    void UpdatePlayer(CPlayer* player, float deltaTime, const PlayerInput& input, float cameraYaw, const CGameMap* gameMap);

    // [새] 발사 버튼을 누르고 있고 다시 쏠 수 있는 시간이 되었으면 총알을 쏜다. 유령은 쏘지 않는다.
    //   인자 player: 쏠 기체
    //   인자 ownerPlayerId: 쏜 플레이어 번호
    //   인자 input: 이 기체의 입력
    void HandlePlayerFire(CPlayer* player, uint8_t ownerPlayerId, const PlayerInput& input);

    // [기존] 적이 가장 가까운 살아 있는 플레이어를 쫓아가 쏜다.
    //   인자 deltaTime: 진행할 시간
    //   인자 gameMap: 맵
    void UpdateEnemies(float deltaTime, const CGameMap* gameMap);

    // [기존] 총알을 움직이고, 최대 거리를 넘거나 땅에 닿으면 끈다.
    //   인자 deltaTime: 진행할 시간
    //   인자 gameMap: 맵
    void UpdateBullets(float deltaTime, const CGameMap* gameMap);

    // [새] 총알이 맞았는지 확인하고 피해를 준다. 이번에 죽은 적과 플레이어를 돌려준다. 플레이어가 쏜
    // 총알은 다른 플레이어도 맞히고(자기 총알은 안 맞음), 쉴드가 켜져 있으면 피해가 없다.
    //   인자 outDeadEnemies: 이번에 죽은 적을 담아 돌려받을 곳
    //   인자 outDeadPlayers: 이번에 죽은 플레이어를 담아 돌려받을 곳
    void CheckBulletHits(std::vector<CEnemy*>& outDeadEnemies, std::vector<CPlayer*>& outDeadPlayers);

    // [새] 살아 있는 플레이어 수를 돌려주고, 살아 있는 플레이어 하나를 outLastAlive에
    // 넣는다(1명 남았을 때 그 사람이 승자). const 함수: 멤버 값을 바꾸지 않는다.
    //   인자 outLastAlive: 마지막 생존자를 담아 돌려받을 변수
    int CountAlivePlayers(CPlayer*& outLastAlive) const;

    // [기존] 기체·적·총알을 모두 지운다.
    void Clear();
};

// ----- CPlayer (문서 3-5, 4-8) -----

// Player.h (기존 파일) — 더하거나 바뀌는 것만 적음
// SetGhost·IsGhost는 서버 계산과 클라 표시에,
// SetHealth·SetShieldEnabled는 클라가 서버에서 받은 값을 넣을 때 쓴다.
class CPlayer : public CGameObject
{
public:
    // [새] 유령(죽은 뒤 관전만 하는 상태)으로 바꾼다. 유령은 움직일 수만 있고, 쏘거나 맞지 않는다.
    // 클라에서는 내 기체가 아닌 유령은 그리지 않는다.
    //   인자 ghost: true면 유령으로
    void SetGhost(bool ghost);

    // [새] 유령이면 true를 돌려준다(const 함수: 멤버 값을 바꾸지 않음).
    bool IsGhost() const;

    // [새] 받은 체력을 넣는다(화면 표시용).
    //   인자 health: 서버에서 받은 체력
    void SetHealth(int health);

    // [새] 받은 쉴드 상태를 넣는다(화면 표시용).
    //   인자 enabled: 서버에서 받은 쉴드 상태
    void SetShieldEnabled(bool enabled);

private:
    // [새] 유령 상태인가(새로 더하는 값)
    bool ghost;
};

// ----- CBullet (문서 3-5) -----

// Bullet.h (기존 파일) — 더하거나 바뀌는 것만 적음
class CBullet : public CGameObject
{
public:
    // [기존, 바뀜] 꺼져 있던 총알을 켜서 쏜다. 쏜 플레이어 번호를 기록해 두어 자기 총알에는 맞지
    // 않게 한다.
    //   인자 startPosition: 나가는 위치
    //   인자 direction: 나가는 방향
    //   인자 newOwner: 플레이어 총알인지 적 총알인지
    //   인자 ownerPlayerId: 쏜 플레이어 번호(새로 더하는 인자)
    void Fire(const XMFLOAT3& startPosition, const XMFLOAT3& direction, EBulletOwner newOwner, uint8_t ownerPlayerId);

    // [새] 이 총알을 쏜 플레이어 번호를 돌려준다(const 함수: 멤버 값을 바꾸지 않음).
    uint8_t GetOwnerPlayerId() const;

private:
    // [새] 쏜 플레이어 번호(새로 더하는 값). 적이 쏜 총알이면 255
    uint8_t ownerPlayerId;
};

// ==========================================================================================
// 3. 서버 — 문서 3장
// ==========================================================================================


// ----- CClientSession (문서 3-2) -----

// GameServer.h (새 파일)
// 접속한 클라 1명의 정보다. CGameServer가 4칸(m_seats[4])을 가지고 있다.
struct CClientSession
{
    // 이 자리에 클라가 있으면 true
    bool             used;
    // 이 클라가 준비 버튼을 눌렀으면 true
    bool             ready;
    // 이 클라와 연결된 TCP 소켓
    SOCKET           sock;
    // 이 자리의 송신 잠금. 두 스레드가 같은 소켓에 동시에 보내서 패킷이 섞이는 것을 막는다
    CRITICAL_SECTION sendLock;
    // 이 클라를 맡은 TCP 통신 스레드의 핸들
    HANDLE           thread;
};

// ----- InputSlot (문서 3-3) -----

// GameServer.h (새 파일)
// 플레이어 1명이 마지막으로 보낸 입력과 그 플레이어의 UDP 주소다. CGameServer가
// 4칸(m_inputs[4])을 가지고 있다.
struct InputSlot
{
    // 이 칸에 플레이어가 있으면 true. false인 칸으로 온 입력은 버린다
    bool        active;
    // 마지막으로 받은 입력의 일련번호. 새로 온 번호가 이 값보다 작거나 같으면 늦게 도착한 옛날 입력이라
    // 버린다. 자리 등록 때 0으로 되돌린다
    uint32_t    lastSequence;
    // 마지막으로 받은 입력
    CS_Input    input;
    // 이 플레이어의 UDP 주소(IP + 포트). 입력을 받을 때 recvfrom이 알려 준다
    sockaddr_in addr;
    // addr를 한 번이라도 받았으면 true. false면 아직 상태를 보낼 주소를 모른다
    bool        hasAddr;
};

// ----- CGameLoop (문서 3-4) -----

// GameLoop.h (새 파일)
// 서버 게임 루프 스레드 하나가 쓰는 클래스다. 게임 한 판을 준비하고, 1/30초마다 게임을
// 진행하고, 끝나면 로비로 되돌린다. 아래 함수는 모두 서버 게임 루프 스레드에서 실행된다.
class CGameLoop
{
public:
    // ===== 서버 게임 루프 =====

    // 게임 루프 스레드가 시작되면 가장 먼저 실행되는 함수다. Run을 부른다.
    //   인자 param: CGameLoop 포인터
    static DWORD WINAPI GameLoopThread(LPVOID param);

    // 게임 루프 스레드가 하는 일 전체다.
    //   ① 시작 신호를 기다린다(WaitForSingleObject)
    //   ② PrepareGame
    //   ③ 게임이 끝날 때까지 WaitNextTick → RunTick → IsGameOver를 반복
    //   ④ SendResult
    //   ⑤ ReturnToLobby
    //   ⑥
    //   ①로 돌아간다.
    void Run();

    // 게임 한 판을 준비한다. 성공하면 true.
    // 하는 순서:
    //   ① 참가자 확인(GetParticipants)
    //   ② 맵(1~3)과 각자의 기체를 무작위로 정한다
    //   ③ 맵과 World를 만든다
    //   ④ 남은 입력 지우기(ClearInputs)
    //   ⑤ 지금의 'E 누른 횟수'를 m_lastShieldCount에 기록
    //   ⑥ 전원에게 SC_GameStart를 보낸다.
    bool PrepareGame();

    // m_timer.Tick(30.0f)를 불러, 지난 틱부터 1/30초가 될 때까지 기다린다.
    // Sleep은 쓰지 않는다.
    void WaitNextTick();

    // 틱 한 번을 진행한다.
    //   ① m_tick을 1 올린다
    //   ② 4명의 입력을 복사해 온다(CopyInputs)
    //   ③ UpdateWorld
    //   ④ 이번 틱에 죽은 것이 있으면 SendEvents
    //   ⑤ SendStates.
    void RunTick();

    // 입력대로 기체를 움직이고 적 AI·총알·피격을 1/30초만큼 계산한다. 이번 틱에 죽은 플레이어는
    // 유령으로 바꾸고, 죽은 플레이어와 적을 outEvents에 넣는다.
    //   인자 inputs: 4명의 입력
    //   인자 outEvents: 이번 틱의 사망 목록을 담아 돌려받을 곳
    void UpdateWorld(const CS_Input inputs[MAX_PLAYERS], std::vector<SC_Event>& outEvents);

    // 네트워크로 받은 입력을 World가 쓰는 입력 모양(PlayerInput, 5-2)으로 바꿔
    // 돌려준다. 'E 누른 횟수'가 지난 틱과 다르면 '이번에 E를
    // 눌렀다'(shieldTogglePressed = true)로 바꾼다.
    //   인자 input: 네트워크로 받은 입력
    //   인자 playerId: 그 입력을 보낸 플레이어 번호
    PlayerInput ToPlayerInput(const CS_Input& input, uint8_t playerId);

    // 사망 이벤트(SC_Event)를 하나씩 전원에게 TCP로 보낸다.
    //   인자 events: 이번 틱의 사망 목록
    void SendEvents(const std::vector<SC_Event>& events);

    // 상태 패킷 3종(플레이어·적·총알)을 채워서, UDP 주소를 아는 클라마다 보낸다(sendto).
    void SendStates();

    // 모든 참가자의 상태를 채우고, 실제로 보낼 바이트 수(8 + 20 × 인원)를 돌려준다.
    //   인자 out: 채울 패킷
    int FillPlayerStates(SC_PlayerStates& out);

    // 살아 있는 적만 채우고, 보낼 바이트 수(8 + 17 × 적 수)를 돌려준다.
    //   인자 out: 채울 패킷
    int FillEnemyStates(SC_EnemyStates& out);

    // 날아가는 총알만 채우고, 보낼 바이트 수(8 + 12 × 총알 수)를 돌려준다.
    //   인자 out: 채울 패킷
    int FillBulletStates(SC_BulletStates& out);

    // 살아 있는 플레이어가 1명 이하면 true를 돌려준다. outWinner에는 살아남은 1명의
    // PlayerId를, 아무도 없으면(동시에 다 죽음) 255를 넣는다.
    //   인자 outWinner: 승자 번호를 담아 돌려받을 변수
    bool IsGameOver(uint8_t& outWinner);

    // 게임 결과(SC_GameResult)를 전원에게 보낸다.
    //   인자 winner: 승자 번호. 없으면 255
    void SendResult(uint8_t winner);

    // World와 맵을 지운다 → ResetLobby → 전원에게 로비 상태를
    // 보낸다(BroadcastLobbyState).
    void ReturnToLobby();

private:
    // 서버 포인터. 자리·입력을 읽고 패킷을 보내는 서버 함수를 쓰기 위해 가지고 있다
    CGameServer*     m_server;
    // 이번 판의 맵(지형 높이, 적이 나타나는 위치)
    CTerrainGameMap* m_map;
    // 게임 세계(기체·적·총알과 그 계산). 게임 루프 스레드만 만지므로 잠금이 필요 없다
    CGameWorld       m_world;
    // 1/30초 간격을 맞추는 타이머
    CGameTimer       m_timer;
    // PlayerId로 그 플레이어의 기체를 찾는 표. 빈 자리는 nullptr
    CPlayer*         m_playerById[MAX_PLAYERS];
    // 지난 틱에 받은 'E 누른 누적 횟수'. 이번 값과 다르면 E를 새로 누른 것이므로 쉴드를 켜거나
    // 끈다
    uint8_t          m_lastShieldCount[MAX_PLAYERS];
    // 틱 번호. 매 틱 1씩 커지고, 게임이 새로 시작돼도 0으로 되돌리지 않는다
    uint32_t         m_tick;
};

// ----- CGameServer (문서 3-1) -----

// GameServer.h (새 파일)
// 서버 프로그램 전체를 대표하는 클래스다. 소켓, 자리 4칸, 입력 4칸, 잠금(임계 영역)을 가지고
// 있고, 서버 메인·서버 TCP 통신·서버 UDP 수신 스레드가 이 클래스의 함수를 실행한다.
class CGameServer
{
public:
    // ===== 서버 메인 =====

    // 서버를 켤 때 한 번 부른다. 모두 성공하면 true, 하나라도 실패하면 false를 돌려준다.
    // 하는 순서:
    //   ① 윈속 시작(WSAStartup)
    //   ② TCP 소켓을 9000번 포트에 열고 접속을 기다리게 한다(bind, listen)
    //   ③ UDP 소켓을 9001번 포트에 연다(bind)
    //   ④ 잠금 2개와 시작 이벤트를 만든다
    //   ⑤ UDP 수신 스레드와 게임 루프 스레드를 시작한다.
    bool Start();

    // 클라의 접속을 기다리다가(accept) 접속이 오면, 그 클라를 맡을 TCP 통신 스레드를 하나
    // 만들고 다시 기다린다. 이것을 계속 반복한다.
    void AcceptLoop();

    // ===== 서버 TCP 통신 (클라 1명당 1개) =====

    // TCP 통신 스레드가 시작되면 가장 먼저 실행되는 함수다(CreateThread가 이 모양을 요구해서
    // static이다). param에서 서버 포인터와 소켓을 꺼내 TcpCommLoop를 부른다.
    //   인자 param: 서버 포인터와 그 클라의 소켓을 담은 값
    static DWORD WINAPI TcpCommThread(LPVOID param);

    // 클라 1명을 맡는 스레드가 하는 일 전체다.
    //   ① TryRegisterSeat로 자리 등록
    //   ② SendConnectResult로 접속 결과 보내기
    //   ③ 승인이면 BroadcastLobbyState로 전원에게 로비 상태 보내기
    //   ④ 그 뒤로는 이 클라가 보내는 패킷(CS_Ready)을 기다렸다가 처리하기를 반복한다.
    //   인자 sock: 이 클라와 연결된 TCP 소켓
    void TcpCommLoop(SOCKET sock);

    // 지금이 로비이고 빈 자리가 있으면, 가장 앞의 빈 자리에 이 클라를 적고 그 자리 번호(0~3 =
    // PlayerId)를 돌려준다. 등록할 수 없으면 255(INVALID_PLAYER_ID)를 돌려주고
    // outReason에 이유를 적는다(ServerFull = 4자리가 다 참, AlreadyStarted
    // = 이미 게임 중).
    //   인자 sock: 이 클라의 TCP 소켓
    //   인자 outReason: 거절 이유를 담아 돌려받을 변수
    uint8_t TryRegisterSeat(SOCKET sock, ERejectReason& outReason);

    // 이 클라에게 접속 결과(SC_ConnectResult)를 보낸다. 거절이면 보낸 뒤 소켓을 닫는다.
    //   인자 sock: 이 클라의 TCP 소켓
    //   인자 playerId: 받은 자리 번호. 거절이면 255
    //   인자 reason: 거절 이유. 승인이면 None
    void SendConnectResult(SOCKET sock, uint8_t playerId, ERejectReason reason);

    // 그 자리를 '준비 완료'(ready = true)로 바꾼다.
    //   인자 playerId: 준비 버튼을 누른 플레이어 번호
    void SetReady(uint8_t playerId);

    // 세 조건을 확인한다: 로비 중인가, 접속한 사람이 2명 이상인가, 접속한 사람이 모두 준비했나. 모두
    // 맞으면 m_inGame을 true로 바꾸고 true를 돌려준다. 확인과 바꾸기를 한 번의 잠금 안에서
    // 하므로, 두 클라가 동시에 준비해도 true는 한 스레드만 받는다. true를 받은 스레드만
    // SetEvent로 게임 루프를 깨운다.
    bool TryStartGame();

    // ===== 서버 UDP 수신 =====

    // UDP로 오는 입력 패킷(CS_Input)을 받기(recvfrom)를 반복한다. 크기·플레이어
    // 번호·일련번호를 검사해서 통과한 것만 그 플레이어 칸에 덮어쓰고, 보낸 쪽 주소도 함께 저장한다.
    void UdpRecvLoop();

    // ===== 여러 스레드가 부름 =====

    // 그 자리의 송신 잠금(sendLock)을 잡은 채 패킷이 다 나갈 때까지 보낸다. 서버의 TCP
    // 전송은 모두 이 함수를 거친다. 실패하면 false를 돌려준다.
    //   인자 s: 보낼 상대의 자리
    //   인자 data: 보낼 패킷
    //   인자 len: 보낼 바이트 수
    bool SendPacket(CClientSession& s, const char* data, int len);

    // 접속한 모든 자리에 SendPacket을 부른다.
    //   인자 data: 보낼 패킷
    //   인자 len: 보낼 바이트 수
    void SendToAll(const char* data, int len);

    // 자리 목록을 보고 로비 상태(SC_LobbyState: 누가 접속했고 누가 준비했나)를 만들어 접속한
    // 전원에게 보낸다.
    void BroadcastLobbyState();

    // ===== 서버 게임 루프가 부름 =====

    // 사람이 있는 자리는 true, 빈 자리는 false로 채워 준다.
    //   인자 outJoined: 결과를 채울 배열
    void GetParticipants(bool outJoined[MAX_PLAYERS]);

    // 4명의 마지막 입력을 입력 잠금 안에서 복사해 준다.
    //   인자 outInputs: 결과를 채울 배열
    void CopyInputs(CS_Input outInputs[MAX_PLAYERS]);

    // 4명의 UDP 주소와, 그 주소를 이미 알고 있는지(첫 입력을 받았는지)를 복사해 준다.
    //   인자 outAddrs: UDP 주소를 채울 배열
    //   인자 outHasAddr: 주소를 알고 있는지를 채울 배열
    void CopyUdpAddrs(sockaddr_in outAddrs[MAX_PLAYERS], bool outHasAddr[MAX_PLAYERS]);

    // 남아 있는 마지막 입력의 이동·발사 값을 0으로 지운다. 지난 판의 입력 때문에 새 판에서 기체가
    // 혼자 움직이지 않게 하려는 것이다. 일련번호·주소·E 누른 횟수는 그대로 둔다.
    void ClearInputs();

    // m_inGame을 false로 바꾸고 모든 자리의 준비를 푼다.
    void ResetLobby();

private:
    // 클라의 접속을 기다리는 TCP 소켓(포트 9000)
    SOCKET           m_listenSock;
    // UDP 소켓(포트 9001). UDP 수신 스레드가 입력을 받을 때, 게임 루프 스레드가 상태를 보낼
    // 때 같이 쓴다
    SOCKET           m_udpSock;
    // 자리 4칸(3-2). 칸 번호 0~3이 그 플레이어의 PlayerId다
    CClientSession   m_seats[MAX_PLAYERS];
    // 지금 게임 중이면 true, 로비(게임 전 대기)면 false
    bool             m_inGame;
    // 자리 잠금. m_seats의 used·ready와 m_inGame을 한 번에 한 스레드만 만지게 한다
    CRITICAL_SECTION m_seatLock;
    // 입력 4칸(3-3). 플레이어별 마지막 입력과 UDP 주소
    InputSlot        m_inputs[MAX_PLAYERS];
    // 입력 잠금. m_inputs를 한 번에 한 스레드만 만지게 한다
    CRITICAL_SECTION m_inputLock;
    // 게임 시작 신호(이벤트). 자동 리셋 방식이라 게임 루프가 신호를 한 번 받으면 저절로 다시 '신호
    // 없음'이 된다
    HANDLE           m_startEvent;
    // 게임 진행을 맡는 객체(3-4)
    CGameLoop        m_gameLoop;
    // UDP 수신 스레드와 게임 루프 스레드의 핸들(만든 스레드를 가리키는 값. 서버를 끌 때 스레드가
    // 끝나길 기다리는 데 쓴다)
    HANDLE           m_udpThread;
    HANDLE           m_gameLoopThread;
};

// ==========================================================================================
// 4. 클라 — 문서 4장
// ==========================================================================================

// 접속할 서버의 IP (전역 상수, NetworkClient.h). 127.0.0.1은 내 PC를 뜻한다.
// 시연 때는 서버 PC의 IP로 바꿔서 빌드한다.
constexpr const char* SERVER_IP = "127.0.0.1";

// ----- CNetworkClient (문서 4-1) -----

// NetworkClient.h (새 파일)
// 클라의 네트워크 일을 모두 맡는다. 소켓 2개(TCP·UDP), 수신 스레드 2개, 이벤트 큐, 최신
// 상태를 가지고 있다. CGameFramework가 하나 가지고 있다.
class CNetworkClient
{
public:
    // ===== 클라 메인가 부름 =====

    // 서버에 TCP로 연결한다. 성공하면 true, 실패하면 만든 것을 정리하고 false를 돌려준다.
    // 하는 순서:
    //   ① 윈속 시작(WSAStartup)
    //   ② TCP 소켓 만들기
    //   ③ 서버의 9000번 포트에 연결(connect).
    //   인자 serverIp: 서버 IP 문자열(SERVER_IP)
    bool Connect(const char* serverIp);

    // 패킷을 받을 준비를 한다. 성공하면 true.
    // 하는 순서:
    //   ① UDP 소켓을 만들고 포트 0으로 bind한다(bind하지 않으면 recvfrom이
    //   WSAEINVAL 오류를 낸다)
    //   ② 서버의 UDP 주소를 저장한다
    //   ③ TCP 수신·UDP 수신 스레드를 시작한다.
    bool StartReceive();

    // 준비 패킷(CS_Ready)을 TCP로 보낸다.
    void SendReady();

    // 내 PlayerId와 새 일련번호(지난 번호 + 1)를 붙여 입력 패킷(CS_Input)을 만들고
    // 서버로 UDP 전송한다(sendto).
    //   인자 moveForward: 앞뒤 이동: -1(S), 0, 1(W)
    //   인자 moveRight: 좌우 이동: -1(A), 0, 1(D)
    //   인자 cameraYaw: 카메라의 좌우 방향(도)
    //   인자 fire: 좌클릭을 누르고 있으면 1
    //   인자 shieldPressCount: E를 누른 누적 횟수
    void SendInput(int8_t moveForward, int8_t moveRight, float cameraYaw, uint8_t fire, uint8_t shieldPressCount);

    // 큐 잠금을 잡고 큐와 out을 통째로 맞바꾼다(swap). 그러면 out에는 쌓여 있던 이벤트가 전부
    // 들어오고 큐는 빈다. 잠금은 맞바꾸는 짧은 순간만 잡는다.
    //   인자 out: 꺼낸 이벤트를 받을 빈 목록
    void PopEvents(std::vector<NetEvent>& out);

    // 상태 잠금을 잡고 최신 상태 3종을 out에 복사한 뒤, '새 값 있음' 표시(hasNew…)를
    // 끈다.
    //   인자 out: 복사해 받을 곳
    void CopyLatestStates(LatestStates& out);

    // 내 PlayerId를 저장한다.
    //   인자 id: 승인 때 받은 내 번호
    void SetMyPlayerId(uint8_t id);

    // 내 PlayerId를 돌려준다(const 함수: 멤버 값을 바꾸지 않음).
    uint8_t GetMyPlayerId() const;

    // 소켓을 닫는다(그러면 수신 스레드의 받기가 실패해서 스레드가 끝난다) → 스레드가 끝나길 기다린다 →
    // 윈속을 정리한다(WSACleanup).
    void Disconnect();

    // ===== 수신 스레드 =====

    // TCP 수신 스레드가 시작되면 가장 먼저 실행되는 함수다. TcpRecvLoop를 부른다.
    //   인자 param: CNetworkClient 포인터
    static DWORD WINAPI TcpRecvThread(LPVOID param);

    // UDP 수신 스레드가 시작되면 가장 먼저 실행되는 함수다. UdpRecvLoop를 부른다.
    //   인자 param: CNetworkClient 포인터
    static DWORD WINAPI UdpRecvThread(LPVOID param);

    // 서버가 보내는 TCP 패킷을 하나씩 받아(RecvPacket) NetEvent로 만들어 큐에 넣기를
    // 반복한다. 연결이 끊기면 끝난다.
    void TcpRecvLoop();

    // UDP 상태 패킷 받기를 반복한다. 크기 검사와 tick 검사(그 종류의 마지막 tick보다 작거나
    // 같으면 버림)를 통과하면 그 종류의 최신 상태에 덮어쓰고 '새 값 있음' 표시를 켠다.
    void UdpRecvLoop();

private:
    // 서버와 연결된 TCP 소켓
    SOCKET                m_tcpSock;
    // UDP 소켓. 포트 0으로 bind한다(0 = 운영체제가 빈 포트를 골라 줌)
    SOCKET                m_udpSock;
    // 입력을 보낼 서버의 UDP 주소(SERVER_IP의 9001번 포트)
    sockaddr_in           m_serverUdpAddr;
    // TCP 수신·UDP 수신 스레드의 핸들
    HANDLE                m_tcpThread;
    HANDLE                m_udpThread;
    // 이벤트 큐. TCP로 받은 패킷을 도착 순서대로 쌓아 둔다
    std::vector<NetEvent> m_eventQueue;
    // 큐 잠금. TCP 수신 스레드(넣기)와 메인 스레드(꺼내기)가 동시에 큐를 만지지 않게 한다
    CRITICAL_SECTION      m_queueLock;
    // UDP로 받은 최신 상태 3종(5-2)
    LatestStates          m_latest;
    // 상태 잠금. UDP 수신 스레드(덮어쓰기)와 메인 스레드(복사)가 동시에 만지지 않게 한다
    CRITICAL_SECTION      m_stateLock;
    // 상태 종류(플레이어·적·총알)별로 마지막으로 받은 tick 번호. UDP 수신 스레드만 쓴다
    uint32_t              m_lastTick[3];
    // 내 PlayerId. 접속이 승인될 때 받는다
    uint8_t               m_myPlayerId;
    // 마지막으로 보낸 입력의 일련번호. 보낼 때마다 1씩 올리고 되돌리지 않는다
    uint32_t              m_inputSequence;
};

// ----- CGameFramework (문서 4-2) -----

// GameFramework.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 클라 메인 스레드에서 매 프레임을 진행하고 화면(씬)을 바꾼다. 받은 TCP 패킷을 꺼내 지금 화면에
// 나눠 준다. 4-2~4-8의 함수는 모두 클라 메인 스레드에서 실행된다.
class CGameFramework
{
public:
    // ===== 클라 메인 =====

    // [기존, 바뀜] 한 프레임을 진행한다.
    //   ① gameTimer.Tick(30.0f)로 지난 프레임부터 1/30초가 될 때까지 기다림
    //   ② ProcessInput(키 입력)
    //   ③ ProcessNetEvents(받은 TCP 패킷 처리)
    //   ④ AnimateObjects(움직임 갱신)
    //   ⑤ 그리기
    //   ⑥ Present(그린 것을 화면에 보이기)
    //   ⑦ CheckSceneChange(화면 바꾸기 요청 처리).
    void FrameAdvance();

    // [새] PopEvents로 큐를 꺼내 하나씩 종류를 보고 처리한다. SC_LobbyState는
    // m_lastLobbyState에 저장하고, 지금이 로비 화면이면 SetLobbyState로 바로
    // 반영한다. SC_GameStart는 m_gameStart에 저장하고 게임 화면으로 바꾼다. 그 밖의
    // 패킷은 지금 화면의 OnNetEvent에 넘긴다.
    void ProcessNetEvents();

    // [기존, 바뀜] 화면을 바꾼다. 게임 화면이면 m_gameStart로
    // GameSessionConfig(5-2)를 채워 넘기고, 로비 화면이면
    // m_lastLobbyState를 넘긴다. 새 화면에 SetNetworkClient로 네트워크 객체를
    // 넣어 준다.
    //   인자 nextScene: 바꿀 화면 종류
    //   인자 buildDirectResources: 그리기 자원을 새로 만들지
    void ChangeScene(ESceneType nextScene, bool buildDirectResources);

    // [기존] 화면이 남겨 둔 '바꿔 달라'는 요청(requestedScene)이 있으면
    // ChangeScene을 부른다. 그리는 도중에 화면이 바뀌지 않도록 Present 뒤에서 부른다.
    void CheckSceneChange();

private:
    // [새] 네트워크를 맡는 객체(4-1)
    CNetworkClient m_network;
    // [새] 가장 최근에 받은 로비 상태. 어느 화면에서 받든 저장해 두었다가 로비 화면을 만들 때 넘긴다
    SC_LobbyState  m_lastLobbyState;
    // [새] 받은 게임 시작 정보(맵, 참가자, 시작 위치). 게임 화면을 만들 때 쓴다
    SC_GameStart   m_gameStart;
};

// ----- CSceneBase (문서 4-3) -----

// SceneBase.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 시작·로비·게임·결과 화면이 모두 이 클래스를 물려받는다. 여기에 더한 것은 화면 4개가 모두
// 가진다.
class CSceneBase
{
public:
    // ===== 클라 메인 =====

    // [새] 화면이 TCP 패킷을 받는 함수다. 기본은 아무것도 하지 않고, 패킷이 필요한 화면(시작·게임
    // 화면)만 다시 만든다(override).
    //   인자 event: 받은 TCP 패킷 하나
    virtual void OnNetEvent(const NetEvent& event);

    // [새] m_network에 넣는다.
    //   인자 network: 네트워크 객체 포인터
    void SetNetworkClient(CNetworkClient* network);

private:
    // [새] 네트워크 객체 포인터. 화면에서 패킷을 보낼 때 쓴다
    CNetworkClient* m_network;
};

// ----- CStartScene (문서 4-4) -----

// StartScene.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 게임을 켜면 처음 나오는 화면이다. 제목을 클릭하면 서버에 접속한다.
class CStartScene : public CSceneBase
{
public:
    // ===== 클라 메인 =====

    // [기존, 바뀜] 제목 글자를 클릭하면 Connect(SERVER_IP)를 부르고, 성공하면
    // StartReceive를 부른다. 이 클릭을 처리했으면 true.
    //   인자 hWnd: 창 핸들
    //   인자 message: 마우스 메시지 종류(클릭 등)
    //   인자 wParam: 메시지 추가 정보
    //   인자 lParam: 마우스 좌표
    bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

    // [새] 접속 결과(SC_ConnectResult)를 처리한다. accepted가 1(승인)이면
    // SetMyPlayerId(받은 번호) 후 로비 화면을 요청한다. 0(거절)이면 제목 글자를 빨간색으로
    // 바꾸고 Disconnect를 부른다. 시작 화면에 그대로 남아 다시 클릭할 수 있다.
    //   인자 event: 받은 TCP 패킷 하나
    void OnNetEvent(const NetEvent& event) override;
};

// ----- CMapSelectScene (문서 4-5) -----

// MapSelectScene.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 게임 전에 모여서 준비 버튼을 누르는 화면이다. 원래 맵 선택 화면이던 것을 로비로 쓴다.
class CMapSelectScene : public CSceneBase
{
public:
    // ===== 클라 메인 =====

    // [기존, 바뀜] 로비 화면을 만든다. m_readySent = false로 시작해서 준비 버튼이
    // 눌리는 상태가 된다.
    //   인자 clientWidth: 창 너비
    //   인자 clientHeight: 창 높이
    void BuildObjects(int clientWidth, int clientHeight) override;

    // [기존, 바뀜] 준비 버튼(Start 글자)을 처음 누르면 SendReady를 부르고 버튼을 어둡게
    // 한다(m_readySent = true). 이 클릭을 처리했으면 true.
    //   인자 hWnd: 창 핸들
    //   인자 message: 마우스 메시지 종류(클릭 등)
    //   인자 wParam: 메시지 추가 정보
    //   인자 lParam: 마우스 좌표
    bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

    // [새] 받은 로비 상태를 저장하고 자리 4칸의 색을 바꾼다. 빈 자리는 회색, 접속은 흰색, 준비는
    // 초록색이고 내 자리는 크게 그린다.
    //   인자 state: 받은 로비 상태
    void SetLobbyState(const SC_LobbyState& state);

    // [기존, 바뀜] 준비 버튼과 자리 4칸을 그린다.
    //   인자 commandList: DirectX12 그리기 명령을 담는 객체
    void RenderDirect(ID3D12GraphicsCommandList* commandList) override;

private:
    // [새] 화면에 그릴 로비 상태(누가 접속했고 누가 준비했나)
    SC_LobbyState m_lobbyState;
    // [새] 준비 버튼을 이미 눌러 CS_Ready를 보냈으면 true. 두 번 보내지 않으려고 둔다
    bool          m_readySent;
};

// ----- CGameScene (문서 4-6) -----

// GameScene.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 게임 중 화면이다. 계산(이동·피격·승패)은 서버가 하므로, 이 화면은 입력을 보내고 서버가 보낸
// 값을 그대로 보여 주기만 한다.
class CGameScene : public CSceneBase
{
public:
    // ===== 클라 메인 =====

    // [기존, 바뀜] 서버가 정한 맵과 참가자 정보로 게임 화면 객체를 만든다.
    //   인자 config: 서버가 정한 맵·참가자·내 번호
    CGameScene(const GameSessionConfig& config);

    // [기존, 바뀜] 맵, 참가자 전원의 기체, 적, 총알을 만들고 그중 내 기체를 정한다.
    //   인자 clientWidth: 창 너비
    //   인자 clientHeight: 창 높이
    void BuildObjects(int clientWidth, int clientHeight) override;

    // [기존, 바뀜] 키보드·마우스로 이번 프레임의 입력을 만들고 SendMyInput을 부른다.
    //   인자 keyBuffer: 키보드 256개 키의 눌림 상태
    bool ProcessInput(UCHAR* keyBuffer) override;

    // [새] 이번 프레임 입력을 패킷 값으로 바꿔(이동 -1/0/1, 카메라 좌우 방향, 발사 0/1, E
    // 누른 누적 횟수) SendInput을 부른다.
    void SendMyInput();

    // [새] 사망 이벤트(SC_Event): 죽은 것이 플레이어면 그 기체를 유령으로 바꾸고, 적이면
    // 지운다. 둘 다 그 위치에 폭발을 보여 준다. 게임 결과(SC_GameResult): 승자 번호가 내
    // 번호면 승리 화면, 아니면(255 포함) 패배 화면을 요청한다.
    //   인자 event: 받은 TCP 패킷 하나
    void OnNetEvent(const NetEvent& event) override;

    // [기존, 바뀜] CopyLatestStates로 최신 상태를 가져와 ApplyStates를 부른 뒤,
    // 카메라·쉴드 색·폭발 연출을 갱신한다.
    //   인자 deltaTime: 지난 프레임부터 흐른 시간(초)
    void AnimateObjects(float deltaTime) override;

    // [새] 새 값이 온 종류만 반영한다. 플레이어는 위치·방향·체력·쉴드·유령 여부, 적은 위치·방향,
    // 총알은 위치를 넣는다. 상태 패킷에 없는 적은 마지막 위치 그대로 둔다(적을 지우는 것은 사망
    // 이벤트뿐).
    //   인자 s: 가져온 최신 상태
    void ApplyStates(const LatestStates& s);

    // [기존, 바뀜] 지형, 기체(유령은 내 것만), 적, 총알, 폭발을 그린다.
    //   인자 commandList: DirectX12 그리기 명령을 담는 객체
    void RenderDirect(ID3D12GraphicsCommandList* commandList) override;

private:
    // [새] PlayerId로 그 플레이어의 기체를 찾는 표
    CPlayer* m_playerById[MAX_PLAYERS];
    // [새] E를 누른 누적 횟수. 누를 때마다 1씩 올려서 서버로 보낸다
    uint8_t  m_shieldPressCount;
};

// ----- CEndScene (문서 4-7) -----

// EndScene.h (기존 파일) — 더하거나 바뀌는 것만 적음
// 게임이 끝나면 승리 또는 패배를 보여 주는 화면이다.
class CEndScene : public CSceneBase
{
public:
    // ===== 클라 메인 =====

    // [기존] true면 'YOU WIN!', false면 'LOSE'를 보여 주는 화면을 만든다.
    //   인자 victory: true면 승리, false면 패배
    CEndScene(bool victory);

    // [기존, 바뀜] 글자를 클릭하면 로비 화면(MapSelect)으로 가는 전환을 요청한다.
    //   인자 hWnd: 창 핸들
    //   인자 message: 마우스 메시지 종류(클릭 등)
    //   인자 wParam: 메시지 추가 정보
    //   인자 lParam: 마우스 좌표
    bool OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};
