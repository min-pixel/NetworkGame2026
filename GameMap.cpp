// 맵 공통 인터페이스 구현 (Spawn 목록 관리)

#include "stdafx.h"
#include "GameMap.h"

CGameMap::CGameMap(int mapNumber)
    : mapNumber(mapNumber)
{
}

int CGameMap::GetMapNumber() const
{
    return mapNumber;
}

const XMFLOAT3& CGameMap::GetPlayerStartPosition() const
{
    return playerStartPosition;
}

const std::vector<FEnemySpawnInfo>& CGameMap::GetEnemySpawns() const
{
    return enemySpawns;
}

void CGameMap::AddEnemySpawn(EEnemyType enemyType, const XMFLOAT3& position)
{
    FEnemySpawnInfo spawnInfo;
    spawnInfo.enemyType = enemyType;
    spawnInfo.position = position;

    enemySpawns.push_back(spawnInfo);
}

void CGameMap::AddEnemySpawn(EEnemyType enemyType, const XMFLOAT3& position, const XMFLOAT4& color)
{
    FEnemySpawnInfo spawnInfo;
    spawnInfo.enemyType = enemyType;
    spawnInfo.position = position;
    spawnInfo.useCustomColor = true;
    spawnInfo.color = color;

    enemySpawns.push_back(spawnInfo);
}
