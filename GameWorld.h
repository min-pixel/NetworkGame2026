// gameplay 시뮬레이션 계층
// Camera / HWND / DirectX 장치를 쓰지 않으며, 나중에 서버에서도 그대로 쓰는 것을 목표로 한다.

#pragma once

#include <vector>
#include "PlayerType.h"
#include "PlayerInput.h"
#include "GameMode.h"

class CPlayer;
class CBullet;
class CEnemy;
class CGameObject;
class CGameMap;

enum class EGameResult
{
    Running,
    Victory,
    Defeat
};

// Player / Enemy / Bullet의 생성, 소유, 삭제와
// 이동, 적 AI, 사격, 지형 충돌, 피격, 피해, Shield, 승패 판정을 담당한다.
class CGameWorld
{
public:
    CGameWorld() = default;
    ~CGameWorld();

    CGameWorld(const CGameWorld&) = delete;
    CGameWorld& operator=(const CGameWorld&) = delete;

    // 모든 gameplay 객체를 삭제하고 결과를 Running으로 되돌린다.
    void Clear();

    // 타입에 맞는 Player를 만들어 지형 높이에 맞춰 놓고 World가 소유한다.
    CPlayer* SpawnPlayer(EPlayerType playerType, const XMFLOAT3& position, const CGameMap* gameMap);
    void AddPlayer(CPlayer* player);

    std::vector<CPlayer*>& GetPlayers();
    const std::vector<CPlayer*>& GetPlayers() const;

    // 이 클라이언트가 조종하는 Player 지정 (players 안의 객체, 비소유)
    void SetLocalPlayer(CPlayer* player);
    CPlayer* GetLocalPlayer() const;
    EPlayerType GetLocalPlayerType() const;

    // Shield 토글, 카메라 yaw 기준 WASD 이동(맵 밖으로는 못 나감), 지형 높이 처리(Tank: 바로 붙음 / Helicopter: 천천히 하강)
    void UpdateLocalPlayer(float deltaTime, const PlayerInput& input, float thirdPersonYaw, const CGameMap* gameMap);

    // 좌클릭 중이면 발사 쿨타임(Helicopter 0.25초, Tank 0.8초)마다 총구 위치에서 기체 정면으로 한 발 발사한다.
    void HandleLocalPlayerFire(const PlayerInput& input);

    // 맵의 Spawn 정보대로 적을 만들어 지형 높이에 맞춰 놓는다.
    void SpawnEnemies(const CGameMap* gameMap);
    void AddEnemy(CEnemy* enemy);

    std::vector<CEnemy*>& GetEnemies();
    const std::vector<CEnemy*>& GetEnemies() const;

    // 각 적이 가장 가까운 생존 Player를 쫓고 공격한다.
    void UpdateEnemies(float deltaTime, const CGameMap* gameMap);

    // Player와 Enemy가 함께 쓰는 총알 풀 (비활성 총알을 재사용)
    void BuildBulletPool(int bulletCount);

    // 총알 이동, 사거리 초과/지형 충돌 시 비활성화
    void UpdateBullets(float deltaTime, const CGameMap* gameMap);

    // Player 총알 -> Enemy, Enemy 총알 -> Player 피격 판정
    // 이번 프레임에 죽은 적은 outDeadEnemies로 돌려준다 (폭발 연출용)
    void CheckBulletHits(std::vector<CEnemy*>& outDeadEnemies);

    std::vector<CBullet*>& GetBullets();
    const std::vector<CBullet*>& GetBullets() const;

    // EliminateAll: 적 전멸 = Victory, Local Player 사망 = Defeat
    void UpdateGameResult(EGameMode gameMode);
    EGameResult GetGameResult() const;

private:
    // 오브젝트를 그 위치의 지형 높이로 옮긴다 (지형 밖이면 그대로)
    void SnapToGround(CGameObject* object, const CGameMap* gameMap) const;

    // 이동 후 지형(맵) 밖이면 X, Z 축 중 맵 안에 남는 쪽만 이동을 살리고 나머지는 되돌린다 (가장자리를 따라 미끄러짐)
    void KeepInsideMap(CGameObject* object, const XMFLOAT3& previousPosition, const CGameMap* gameMap) const;

    // 수평(XZ) 거리 기준 가장 가까운 생존 Player
    CPlayer* FindNearestAlivePlayer(const XMFLOAT3& position) const;
    bool IsShieldBlocking(const CPlayer* player) const;

    // 대상을 바라보고, 멀면 접근 / 너무 가까우면 후퇴, 사거리 안이면 발사
    void UpdateEnemyAI(CEnemy* enemy, CPlayer* target, float deltaTime, const CGameMap* gameMap);

private:
    std::vector<CPlayer*> players;

    // players 중 하나를 가리키는 비소유 포인터
    CPlayer* localPlayer = nullptr;

    std::vector<CEnemy*> enemies;
    std::vector<CBullet*> bullets;

    EGameResult gameResult = EGameResult::Running;
};
