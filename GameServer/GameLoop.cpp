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

    int mapNumber = 3;
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

    CS_Input inputs[MAX_PLAYERS];
    m_server->CopyInputs(inputs);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_lastShieldCount[i] = inputs[i].shieldPressCount;
    }

    m_timer.Reset();

    m_server->SendToAll((const char*)&packet, sizeof(packet));
    printf("게임 준비 (맵 %d, %d명)\n", mapNumber, packet.playerCount);
    return true;
}

void CGameLoop::WaitNextTick()
{
    m_timer.Tick((float)TICK_RATE);
}

void CGameLoop::UpdateWorld(const CS_Input inputs[MAX_PLAYERS], std::vector<SC_Event>& outEvents)
{
    float deltaTime = 1.0f / TICK_RATE;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!m_playerById[i])
        {
            continue;
        }

        PlayerInput input = ToPlayerInput(inputs[i]);
        uint8_t shieldCount = (uint8_t)(inputs[i].shieldPressCount - m_lastShieldCount[i]);

        m_world.UpdatePlayer((uint8_t)i, deltaTime, input, inputs[i].cameraYaw, shieldCount, m_map);
    }
}

PlayerInput CGameLoop::ToPlayerInput(const CS_Input& input)
{
    PlayerInput out;
    out.moveForward = (float)input.moveForward;
    out.moveRight = (float)input.moveRight;
    out.fire = (input.fire != 0);
    return out;
}