#include "GameLoop.h"
#include "GameServer.h"
#include "TerrainGameMap.h"
#include "Player.h"
#include <cstdio>

DWORD WINAPI CGameLoop::GameLoopThread(LPVOID param)
{
    CGameLoop* loop = (CGameLoop*)param;

    while (!loop->PrepareGame())
    {
        Sleep(1000);
    }

    return 0;
}

bool CGameLoop::PrepareGame()
{
    bool joined[MAX_PLAYERS];
    m_server->GetParticipants(joined);

    int playerCount = 0;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (joined[i])
        {
            ++playerCount;
        }
    }

    if (playerCount == 0)
    {
        return false;
    }

    int mapNumber = 1;
    EPlayerType playerType = EPlayerType::Helicopter;

    m_map = new CTerrainGameMap(mapNumber);

    if (!m_map->LoadMapData())
    {
        printf("맵 읽기 실패\n");
        delete m_map;
        m_map = nullptr;
        return false;
    }

    m_map->LoadSpawnPositions();

    SC_GameStart packet = {};
    packet.header.size = sizeof(packet);
    packet.header.type = (uint8_t)EPacketType::SC_GameStart;
    packet.mapNumber = (uint8_t)mapNumber;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!joined[i])
        {
            continue;
        }

        XMFLOAT3 startPosition = m_map->GetPlayerStartPosition();
        m_map->GetPlayerStartPosition((uint8_t)i, startPosition);

        CPlayer* player = m_world.SpawnPlayer(playerType, startPosition, m_map, (uint8_t)i);

        if (!player)
        {
            continue;
        }

        m_playerById[i] = player;

        XMFLOAT3 position = player->GetPosition();
        PlayerStartInfo& info = packet.players[packet.playerCount];
        info.playerId = (uint8_t)i;
        info.playerType = (uint8_t)playerType;
        info.x = position.x;
        info.y = position.y;
        info.z = position.z;

        ++packet.playerCount;
    }

    m_world.SpawnEnemies(m_map);
    m_world.BuildBulletPool(MAX_BULLETS);

    //Sleep(10000);

    m_server->SendToAll((const char*)&packet, sizeof(packet));
    printf("게임 준비 (맵 %d, %d명)\n", mapNumber, packet.playerCount);
    return true;
}