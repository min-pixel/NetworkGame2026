#include "GameLoop.h"
#include "GameServer.h"
#include "TerrainGameMap.h"
#include "Player.h"

#include <cstdio>
#include <random>

DWORD WINAPI CGameLoop::GameLoopThread(LPVOID param)
{
    CGameLoop* loop = (CGameLoop*)param;

    loop->Run();

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

    static std::mt19937 randomGenerator(std::random_device{}());
    int mapNumber = std::uniform_int_distribution<int>(1, 3)(randomGenerator);
    EPlayerType playerType = (EPlayerType)std::uniform_int_distribution<int>(1, 2)(randomGenerator);

    int startOffset = std::uniform_int_distribution<int>(0, 3)(randomGenerator);

    m_map = new CTerrainGameMap(mapNumber);

    if (!m_map->LoadMapData())
    {
        printf("∏  ¿–±‚ Ω«∆–\n");
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
        m_map->GetPlayerStartPosition((uint8_t)((i + startOffset) % MAX_PLAYERS), startPosition);

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
    //printf("∞‘¿” ¡ÿ∫Ò (∏  %d, %d∏Ì)\n", mapNumber, packet.playerCount);
    printf("∞‘¿” ¡ÿ∫Ò (∏  %d, ±‚√º %d, %d∏Ì)\n", mapNumber, (int)playerType, packet.playerCount);
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



void CGameLoop::Run()
{
    while (!m_server->TryStartGame())
    {
        Sleep(1000);
    }

    if (!PrepareGame())
    {
        return;
    }

    while (true)
    {
        WaitNextTick();
        RunTick();
    }
}

void CGameLoop::RunTick()
{
    ++m_tick;
    
    //if (m_tick % TICK_RATE == 0) printf("∆Ω %u\n", m_tick);

    CS_Input inputs[MAX_PLAYERS];
    m_server->CopyInputs(inputs);

    std::vector<SC_Event> events;
    UpdateWorld(inputs, events);

    SendStates();
}

void CGameLoop::SendStates()
{
    SC_PlayerStates playerStates;
    int playerBytes = FillPlayerStates(playerStates);

    sockaddr_in addrs[MAX_PLAYERS];
    bool hasAddr[MAX_PLAYERS];
    m_server->CopyUdpAddrs(addrs, hasAddr);

    SOCKET udpSock = m_server->GetUdpSocket();

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!hasAddr[i])
        {
            continue;
        }

        sendto(udpSock, (const char*)&playerStates, playerBytes, 0, (sockaddr*)&addrs[i], sizeof(addrs[i]));
    }
}

int CGameLoop::FillPlayerStates(SC_PlayerStates& out)
{
    out = {};
    out.header.type = (uint8_t)EPacketType::SC_PlayerStates;
    out.tick = m_tick;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        CPlayer* player = m_playerById[i];

        if (!player)
        {
            continue;
        }

        XMFLOAT3 position = player->GetPosition();
        PlayerStateInfo& info = out.players[out.count];
        info.playerId = (uint8_t)i;
        info.state = (uint8_t)EPlayerState::Alive;
        info.shieldEnabled = player->IsShieldEnabled() ? 1 : 0;
        info.health = (uint8_t)player->GetHealth();
        info.x = position.x;
        info.y = position.y;
        info.z = position.z;
        info.yaw = player->GetRotation().y;

        ++out.count;
    }

    int bytes = STATE_PACKET_HEAD_SIZE + (int)sizeof(PlayerStateInfo) * out.count;
    out.header.size = (uint16_t)bytes;
    return bytes;
}