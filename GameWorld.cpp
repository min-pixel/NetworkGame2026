// gameplay 시뮬레이션 구현

#include "stdafx.h"
#include "GameWorld.h"

#include "Player.h"
#include "TankPlayer.h"
#include "HelicopterPlayer.h"
#include "Bullet.h"
#include "Npc.h"
#include "Tank.h"

#include "GameMap.h"
#include "Object.h"
#include "Collider.h"

CGameWorld::~CGameWorld()
{
    Clear();
}

void CGameWorld::Clear()
{
    for (CEnemy* enemy : enemies)
    {
        delete enemy;
    }

    enemies.clear();

    for (CBullet* bullet : bullets)
    {
        delete bullet;
    }

    bullets.clear();

    for (CPlayer* player : players)
    {
        delete player;
    }

    players.clear();

    localPlayer = nullptr;

    gameResult = EGameResult::Running;
}

// ===== 공통 도우미 =====
void CGameWorld::SnapToGround(CGameObject* object, const CGameMap* gameMap) const
{
    if (!object || !gameMap)
    {
        return;
    }

    XMFLOAT3 position = object->GetPosition();

    float groundY = 0.0f;

    if (gameMap->TryGetGroundYAtPosition(position, groundY))
    {
        object->SetPosition(position.x, groundY, position.z);
    }
}

void CGameWorld::KeepInsideMap(CGameObject* object, const XMFLOAT3& previousPosition, const CGameMap* gameMap) const
{
    if (!object || !gameMap)
    {
        return;
    }

    const XMFLOAT3 currentPosition = object->GetPosition();

    float groundY = 0.0f;

    if (gameMap->TryGetGroundYAtPosition(currentPosition, groundY))
    {
        return;
    }

    const XMFLOAT3 moveXOnly(currentPosition.x, currentPosition.y, previousPosition.z);

    if (gameMap->TryGetGroundYAtPosition(moveXOnly, groundY))
    {
        object->SetPosition(moveXOnly.x, moveXOnly.y, moveXOnly.z);
        return;
    }

    const XMFLOAT3 moveZOnly(previousPosition.x, currentPosition.y, currentPosition.z);

    if (gameMap->TryGetGroundYAtPosition(moveZOnly, groundY))
    {
        object->SetPosition(moveZOnly.x, moveZOnly.y, moveZOnly.z);
        return;
    }

    object->SetPosition(previousPosition.x, currentPosition.y, previousPosition.z);
}

CPlayer* CGameWorld::FindNearestAlivePlayer(const XMFLOAT3& position) const
{
    CPlayer* nearestPlayer = nullptr;
    float nearestDistanceSq = FLT_MAX;

    for (CPlayer* player : players)
    {
        if (!player || !player->IsActive() || player->IsDead())
        {
            continue;
        }

        const XMFLOAT3& playerPosition = player->GetPosition();

        float dx = playerPosition.x - position.x;
        float dz = playerPosition.z - position.z;
        float distanceSq = dx * dx + dz * dz;

        if (distanceSq < nearestDistanceSq)
        {
            nearestDistanceSq = distanceSq;
            nearestPlayer = player;
        }
    }

    return nearestPlayer;
}

bool CGameWorld::IsShieldBlocking(const CPlayer* player) const
{
    return player && player->IsShieldEnabled();
}

// ===== Player =====
CPlayer* CGameWorld::SpawnPlayer(EPlayerType playerType, const XMFLOAT3& position, const CGameMap* gameMap)
{
    CPlayer* player = nullptr;

    switch (playerType)
    {
    case EPlayerType::Helicopter:
        player = new CHelicopterPlayer();
        break;

    case EPlayerType::Tank:
        player = new CTankPlayer();
        break;

    default:
        return nullptr;
    }

    player->SetPosition(position.x, position.y, position.z);

    SnapToGround(player, gameMap);

    player->Update(0.0f);

    AddPlayer(player);

    return player;
}

void CGameWorld::AddPlayer(CPlayer* player)
{
    if (!player)
    {
        return;
    }

    for (CPlayer* existingPlayer : players)
    {
        if (existingPlayer == player)
        {
            return;
        }
    }

    players.push_back(player);
}

std::vector<CPlayer*>& CGameWorld::GetPlayers()
{
    return players;
}

const std::vector<CPlayer*>& CGameWorld::GetPlayers() const
{
    return players;
}

void CGameWorld::SetLocalPlayer(CPlayer* player)
{
    localPlayer = player;
}

CPlayer* CGameWorld::GetLocalPlayer() const
{
    return localPlayer;
}

EPlayerType CGameWorld::GetLocalPlayerType() const
{
    if (!localPlayer)
    {
        return EPlayerType::None;
    }

    return localPlayer->GetPlayerType();
}

void CGameWorld::UpdateLocalPlayer(float deltaTime, const PlayerInput& input, float thirdPersonYaw, const CGameMap* gameMap)
{
    if (!localPlayer || !localPlayer->IsActive() || localPlayer->IsDead())
    {
        return;
    }

    const EPlayerType playerType = localPlayer->GetPlayerType();

    if (input.shieldTogglePressed)
    {
        localPlayer->ToggleShield();
    }

    float yawRadians = XMConvertToRadians(thirdPersonYaw);

    XMFLOAT3 forward(sinf(yawRadians), 0.0f, cosf(yawRadians));
    XMFLOAT3 right(cosf(yawRadians), 0.0f, -sinf(yawRadians));

    XMFLOAT3 moveDirection = Vector3::Add(Vector3::Multiply(forward, input.moveForward), Vector3::Multiply(right, input.moveRight));

    const XMFLOAT3 previousPosition = localPlayer->GetPosition();

    localPlayer->MoveByDirection(moveDirection, deltaTime);

    KeepInsideMap(localPlayer, previousPosition, gameMap);

    if (playerType == EPlayerType::Tank)
    {
        SnapToGround(localPlayer, gameMap);
    }

    localPlayer->Update(deltaTime);

    if (playerType == EPlayerType::Tank)
    {
        return;
    }

    if (!gameMap)
    {
        return;
    }

    XMFLOAT3 currentPosition = localPlayer->GetPosition();

    float groundY = 0.0f;

    bool hasGround = gameMap->TryGetGroundYAtPosition(currentPosition, groundY);

    const float fallSpeed = 35.0f;
    const float groundSnapTolerance = 0.05f;

    if (hasGround)
    {
        if (currentPosition.y > groundY + groundSnapTolerance)
        {
            currentPosition.y -= fallSpeed * deltaTime;

            if (currentPosition.y < groundY)
            {
                currentPosition.y = groundY;
            }

            localPlayer->SetPosition(currentPosition.x, currentPosition.y, currentPosition.z);

            localPlayer->UpdateTransform();
            localPlayer->UpdateCollider();
        }
        else if (currentPosition.y < groundY)
        {
            localPlayer->SetPosition(currentPosition.x, groundY, currentPosition.z);

            localPlayer->UpdateTransform();
            localPlayer->UpdateCollider();
        }
    }
    else
    {
        currentPosition.y -= fallSpeed * deltaTime;

        localPlayer->SetPosition(currentPosition.x, currentPosition.y, currentPosition.z);

        localPlayer->UpdateTransform();
        localPlayer->UpdateCollider();
    }
}

void CGameWorld::HandleLocalPlayerFire(const PlayerInput& input)
{
    if (!localPlayer || !localPlayer->IsActive() || localPlayer->IsDead())
    {
        return;
    }

    if (!input.fire)
    {
        return;
    }

    if (!localPlayer->CanFire())
    {
        return;
    }

    XMFLOAT3 fireDirection = localPlayer->GetForward();

    if (Vector3::Length(fireDirection) <= 0.0f)
    {
        return;
    }

    XMFLOAT3 normalizedDirection = Vector3::Normalize(fireDirection);

    XMFLOAT3 startPosition = localPlayer->GetMuzzlePosition();

    for (CBullet* bullet : bullets)
    {
        if (!bullet || bullet->IsActive())
        {
            continue;
        }

        bullet->Fire(startPosition, normalizedDirection, EBulletOwner::Player);

        localPlayer->ResetFireCooldown();

        break;
    }
}

// ===== Enemy =====
void CGameWorld::SpawnEnemies(const CGameMap* gameMap)
{
    if (!gameMap || !enemies.empty())
    {
        return;
    }

    for (const FEnemySpawnInfo& spawnInfo : gameMap->GetEnemySpawns())
    {
        CEnemy* enemy = nullptr;

        switch (spawnInfo.enemyType)
        {
        case EEnemyType::Humanoid:
            enemy = new CNpc();
            break;

        case EEnemyType::Tank:
            enemy = new CTankObject();
            break;
        }

        if (!enemy)
        {
            continue;
        }

        enemy->SetPosition(spawnInfo.position.x, spawnInfo.position.y, spawnInfo.position.z);

        if (spawnInfo.useCustomColor)
        {
            enemy->SetEnemyColor(spawnInfo.color);
        }

        SnapToGround(enemy, gameMap);

        enemy->Update(0.0f);

        AddEnemy(enemy);
    }
}

void CGameWorld::AddEnemy(CEnemy* enemy)
{
    if (!enemy)
    {
        return;
    }

    enemies.push_back(enemy);
}

std::vector<CEnemy*>& CGameWorld::GetEnemies()
{
    return enemies;
}

const std::vector<CEnemy*>& CGameWorld::GetEnemies() const
{
    return enemies;
}

void CGameWorld::UpdateEnemies(float deltaTime, const CGameMap* gameMap)
{
    for (CEnemy* enemy : enemies)
    {
        if (!enemy || !enemy->IsActive() || enemy->IsDead())
        {
            continue;
        }

        CPlayer* target = FindNearestAlivePlayer(enemy->GetPosition());

        if (!target)
        {
            continue;
        }

        UpdateEnemyAI(enemy, target, deltaTime, gameMap);
    }
}

void CGameWorld::UpdateEnemyAI(CEnemy* enemy, CPlayer* target, float deltaTime, const CGameMap* gameMap)
{
    if (!enemy || !target || !enemy->IsActive() || enemy->IsDead() || !target->IsActive() || target->IsDead())
    {
        return;
    }

    XMFLOAT3 targetPosition = target->GetPosition();
    XMFLOAT3 enemyPosition = enemy->GetPosition();

    XMFLOAT3 directionToTarget = Vector3::Subtract(targetPosition, enemyPosition);

    directionToTarget.y = 0.0f;

    float distance = Vector3::Length(directionToTarget);

    if (distance <= 0.01f)
    {
        enemy->Update(deltaTime);

        return;
    }

    directionToTarget = Vector3::Normalize(directionToTarget);

    float yaw = XMConvertToDegrees(atan2f(directionToTarget.x, directionToTarget.z));

    enemy->SetRotation(0.0f, yaw, 0.0f);

    XMFLOAT3 moveDirection(0.0f, 0.0f, 0.0f);

    if (distance > enemy->GetMoveStopDistance())
    {
        moveDirection = directionToTarget;
    }
    else if (distance < enemy->GetTooCloseDistance())
    {
        moveDirection = Vector3::Multiply(directionToTarget, -1.0f);
    }

    if (Vector3::Length(moveDirection) > 0.0f)
    {
        const XMFLOAT3 previousPosition = enemy->GetPosition();

        enemyPosition = Vector3::Add(enemyPosition, Vector3::Multiply(moveDirection, enemy->GetMoveSpeed() * deltaTime));

        enemy->SetPosition(enemyPosition.x, enemyPosition.y, enemyPosition.z);

        KeepInsideMap(enemy, previousPosition, gameMap);
    }

    SnapToGround(enemy, gameMap);

    enemy->Update(deltaTime);

    if (distance > enemy->GetAttackRange())
    {
        return;
    }

    if (!enemy->CanFire())
    {
        return;
    }

    XMFLOAT3 fireStartPosition = Vector3::Add(enemy->GetPosition(), XMFLOAT3(0.0f, 1.8f, 0.0f));
    XMFLOAT3 targetFirePosition = Vector3::Add(target->GetPosition(), XMFLOAT3(0.0f, 1.8f, 0.0f));

    XMFLOAT3 fireDirection = Vector3::Subtract(targetFirePosition, fireStartPosition);

    if (Vector3::Length(fireDirection) <= 0.0f)
    {
        return;
    }

    fireDirection = Vector3::Normalize(fireDirection);

    fireStartPosition = Vector3::Add(fireStartPosition, Vector3::Multiply(fireDirection, 4.0f));

    for (CBullet* bullet : bullets)
    {
        if (!bullet || bullet->IsActive())
        {
            continue;
        }

        bullet->Fire(fireStartPosition, fireDirection, EBulletOwner::Enemy);

        enemy->ResetFireCooldown();

        break;
    }
}

// ===== Bullet =====
void CGameWorld::BuildBulletPool(int bulletCount)
{
    if (!bullets.empty())
    {
        return;
    }

    if (bulletCount <= 0)
    {
        return;
    }

    bullets.reserve(bulletCount);

    for (int i = 0; i < bulletCount; ++i)
    {
        bullets.push_back(new CBullet());
    }
}

void CGameWorld::UpdateBullets(float deltaTime, const CGameMap* gameMap)
{
    for (CBullet* bullet : bullets)
    {
        if (!bullet || !bullet->IsActive())
        {
            continue;
        }

        bullet->Update(deltaTime);

        if (!bullet->IsActive())
        {
            continue;
        }

        if (!gameMap)
        {
            continue;
        }

        const XMFLOAT3& oldPosition = bullet->GetPreviousPosition();
        XMFLOAT3 newPosition = bullet->GetPosition();

        const int terrainSampleCount = 8;
        const float terrainHitOffset = 0.2f;

        bool hitTerrain = false;

        for (int i = 0; i <= terrainSampleCount; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(terrainSampleCount);

            XMFLOAT3 samplePosition;
            samplePosition.x = oldPosition.x + (newPosition.x - oldPosition.x) * t;
            samplePosition.y = oldPosition.y + (newPosition.y - oldPosition.y) * t;
            samplePosition.z = oldPosition.z + (newPosition.z - oldPosition.z) * t;

            float groundY = 0.0f;

            if (gameMap->TryGetGroundYAtPosition(samplePosition, groundY))
            {
                if (samplePosition.y <= groundY + terrainHitOffset)
                {
                    hitTerrain = true;
                    break;
                }
            }
            else
            {
                hitTerrain = true;
                break;
            }
        }

        if (hitTerrain)
        {
            bullet->SetActive(false);
        }
    }
}

void CGameWorld::CheckBulletHits(std::vector<CEnemy*>& outDeadEnemies)
{
    for (CBullet* bullet : bullets)
    {
        if (!bullet || !bullet->IsActive() || !bullet->GetCollider())
        {
            continue;
        }

        if (bullet->GetOwner() == EBulletOwner::Player)
        {
            for (CEnemy* enemy : enemies)
            {
                if (!enemy || !enemy->IsActive() || enemy->IsDead() || !enemy->GetCollider())
                {
                    continue;
                }

                if (!bullet->GetCollider()->Intersects(enemy->GetCollider()))
                {
                    continue;
                }

                int damage = 0;

                if (enemy->GetEnemyType() == EEnemyType::Humanoid)
                {
                    CNpc* npc = static_cast<CNpc*>(enemy);

                    bool hit = false;

                    auto checkPart = [&](CBodyPartObject* part, int partDamage)
                        {
                            if (!part || !part->GetCollider() || hit)
                            {
                                return;
                            }

                            if (bullet->GetCollider()->Intersects(part->GetCollider()))
                            {
                                hit = true;
                                damage = partDamage;
                            }
                        };

                    checkPart(npc->GetHead(), 30);
                    checkPart(npc->GetTorso(), 15);
                    checkPart(npc->GetLeftArm(), 10);
                    checkPart(npc->GetRightArm(), 10);
                    checkPart(npc->GetLeftLeg(), 10);
                    checkPart(npc->GetRightLeg(), 10);

                    if (!hit)
                    {
                        continue;
                    }
                }

                else if (enemy->GetEnemyType() == EEnemyType::Tank)
                {
                    damage = bullet->GetDamage();
                }

                if (damage <= 0)
                {
                    continue;
                }

                enemy->ApplyDamage(damage);

                bullet->SetActive(false);

                if (enemy->IsDead())
                {
                    outDeadEnemies.push_back(enemy);
                }

                break;
            }
        }

        else if (bullet->GetOwner() == EBulletOwner::Enemy)
        {
            for (CPlayer* player : players)
            {
                if (!player || !player->IsActive() || player->IsDead() || !player->GetCollider())
                {
                    continue;
                }

                if (!bullet->GetCollider()->Intersects(player->GetCollider()))
                {
                    continue;
                }

                bullet->SetActive(false);

                if (!IsShieldBlocking(player))
                {
                    player->ApplyDamage(bullet->GetDamage());
                }

                break;
            }
        }
    }
}

// ===== Game Result / 접근자 =====
std::vector<CBullet*>& CGameWorld::GetBullets()
{
    return bullets;
}

const std::vector<CBullet*>& CGameWorld::GetBullets() const
{
    return bullets;
}

EGameResult CGameWorld::GetGameResult() const
{
    return gameResult;
}

void CGameWorld::UpdateGameResult(EGameMode gameMode)
{
    if (gameResult != EGameResult::Running)
    {
        return;
    }

    if (localPlayer && localPlayer->IsDead())
    {
        gameResult = EGameResult::Defeat;

        return;
    }

    switch (gameMode)
    {
    case EGameMode::EliminateAll:
    {
        bool hasEnemy = false;
        bool hasAliveEnemy = false;

        for (CEnemy* enemy : enemies)
        {
            if (!enemy)
            {
                continue;
            }

            hasEnemy = true;

            if (enemy->IsActive() && !enemy->IsDead())
            {
                hasAliveEnemy = true;
                break;
            }
        }

        if (hasEnemy && !hasAliveEnemy)
        {
            gameResult = EGameResult::Victory;
        }

        break;
    }

    case EGameMode::FreeForAll:
    {
        break;
    }

    case EGameMode::None:
    default:
        break;
    }
}
