#include "GameLoop.h"
#include "GameServer.h"
#include "TerrainGameMap.h"
#include "Player.h"
#include "Bullet.h"

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

    m_server->ClearInputs();

    CS_Input inputs[MAX_PLAYERS];
    m_server->CopyInputs(inputs);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_lastShieldCount[i] = inputs[i].shieldPressCount;
    }

    m_timer.Reset();

    m_server->SendToAll((const char*)&packet, sizeof(packet));
    //printf("게임 준비 (맵 %d, %d명)\n", mapNumber, packet.playerCount);
    printf("게임 준비 (맵 %d, 기체 %d, %d명)\n", mapNumber, (int)playerType, packet.playerCount);
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
        m_world.HandlePlayerFire(m_playerById[i], (uint8_t)i, input);
    }

    m_world.UpdateEnemies(deltaTime, m_map);
    m_world.UpdateBullets(deltaTime, m_map);

    std::vector<CEnemy*> deadEnemies;
    m_world.CheckBulletHits(deadEnemies);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        CPlayer* player = m_playerById[i];

        if (!player || !player->IsDead() || player->IsGhost())
        {
            continue;
        }

        player->SetGhost(true);

        XMFLOAT3 position = player->GetPosition();
        SC_Event event = {};
        event.header.size = sizeof(SC_Event);
        event.header.type = (uint8_t)EPacketType::SC_Event;
        event.eventType = (uint8_t)EEventType::PlayerDied;
        event.subjectId = (uint8_t)i;
        event.x = position.x;
        event.y = position.y;
        event.z = position.z;
        outEvents.push_back(event);
    }

    std::vector<CEnemy*>& enemies = m_world.GetEnemies();

    for (CEnemy* deadEnemy : deadEnemies)
    {
        for (int i = 0; i < (int)enemies.size(); ++i)
        {
            if (enemies[i] != deadEnemy)
            {
                continue;
            }

            XMFLOAT3 position = deadEnemy->GetPosition();
            SC_Event event = {};
            event.header.size = sizeof(SC_Event);
            event.header.type = (uint8_t)EPacketType::SC_Event;
            event.eventType = (uint8_t)EEventType::EnemyDied;
            event.subjectId = (uint8_t)i;
            event.x = position.x;
            event.y = position.y;
            event.z = position.z;
            outEvents.push_back(event);
            break;
        }
    }

    for (const SC_Event& event : outEvents) printf("틱 %u: %s %d번 사망\n", m_tick, event.eventType == (uint8_t)EEventType::PlayerDied ? "플레이어" : "적", event.subjectId);




    if (!deadEnemies.empty()) printf("틱 %u: 적 %d마리 사망\n", m_tick, (int)deadEnemies.size());

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

        uint8_t winnerId = INVALID_PLAYER_ID;

        if (IsGameOver(winnerId))
        {
            SendResult(winnerId);
            //break;
        }
    }
}

void CGameLoop::RunTick()
{
    ++m_tick;
    
    //if (m_tick % TICK_RATE == 0) printf("틱 %u\n", m_tick);

    CS_Input inputs[MAX_PLAYERS];
    m_server->CopyInputs(inputs);

    std::vector<SC_Event> events;
    UpdateWorld(inputs, events);

    SendEvents(events);

   


    SendStates();
}

void CGameLoop::SendStates()
{
    SC_PlayerStates playerStates;
    int playerBytes = FillPlayerStates(playerStates);

    SC_EnemyStates enemyStates;
    int enemyBytes = FillEnemyStates(enemyStates);

    SC_BulletStates bulletStates;
    int bulletBytes = FillBulletStates(bulletStates);

    sockaddr_in addrs[MAX_PLAYERS];
    bool hasAddr[MAX_PLAYERS];
    m_server->CopyUdpAddrs(addrs, hasAddr);

    SOCKET udpSock = m_server->GetUdpSocket();

    //if (m_tick % TICK_RATE == 0) printf("틱 %u: 적 %d마리, %d바이트\n", m_tick, enemyStates.count, enemyBytes);
    if (m_tick % TICK_RATE == 0) printf("틱 %u: 적 %d마리 %d바이트, 총알 %d발 %d바이트\n", m_tick, enemyStates.count, enemyBytes, bulletStates.count, bulletBytes);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!hasAddr[i])
        {
            continue;
        }

        sendto(udpSock, (const char*)&playerStates, playerBytes, 0, (sockaddr*)&addrs[i], sizeof(addrs[i]));

        sendto(udpSock, (const char*)&enemyStates, enemyBytes, 0, (sockaddr*)&addrs[i], sizeof(addrs[i]));


        sendto(udpSock, (const char*)&bulletStates, bulletBytes, 0, (sockaddr*)&addrs[i], sizeof(addrs[i]));
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
        //info.state = (uint8_t)EPlayerState::Alive;
        info.state = (uint8_t)(player->IsGhost() ? EPlayerState::Ghost : EPlayerState::Alive);
        
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


int CGameLoop::FillEnemyStates(SC_EnemyStates& out)
{
    out = {};
    out.header.type = (uint8_t)EPacketType::SC_EnemyStates;
    out.tick = m_tick;

    std::vector<CEnemy*>& enemies = m_world.GetEnemies();

    for (int i = 0; i < (int)enemies.size() && i < MAX_ENEMIES; ++i)
    {
        CEnemy* enemy = enemies[i];

        if (!enemy || !enemy->IsActive() || enemy->IsDead())
        {
            continue;
        }

        XMFLOAT3 position = enemy->GetPosition();
        EnemyStateInfo& info = out.enemies[out.count];
        info.enemyIndex = (uint8_t)i;
        info.x = position.x;
        info.y = position.y;
        info.z = position.z;
        info.yaw = enemy->GetRotation().y;

        ++out.count;
    }

    int bytes = STATE_PACKET_HEAD_SIZE + (int)sizeof(EnemyStateInfo) * out.count;
    out.header.size = (uint16_t)bytes;
    return bytes;
}


int CGameLoop::FillBulletStates(SC_BulletStates& out)
{
    out = {};
    out.header.type = (uint8_t)EPacketType::SC_BulletStates;
    out.tick = m_tick;

    std::vector<CBullet*>& bullets = m_world.GetBullets();

    for (int i = 0; i < (int)bullets.size() && out.count < MAX_BULLETS; ++i)
    {
        CBullet* bullet = bullets[i];

        if (!bullet || !bullet->IsActive())
        {
            continue;
        }

        XMFLOAT3 position = bullet->GetPosition();
        BulletStateInfo& info = out.bullets[out.count];
        info.x = position.x;
        info.y = position.y;
        info.z = position.z;

        ++out.count;
    }

    int bytes = STATE_PACKET_HEAD_SIZE + (int)sizeof(BulletStateInfo) * out.count;
    out.header.size = (uint16_t)bytes;
    return bytes;
}

void CGameLoop::SendEvents(const std::vector<SC_Event>& events)
{
    for (const SC_Event& event : events)
    {
        m_server->SendToAll((const char*)&event, sizeof(event));

        printf("틱 %u: %s %d번 사망 전송\n", m_tick, event.eventType == (uint8_t)EEventType::PlayerDied ? "플레이어" : "적", event.subjectId);

    }
}


bool CGameLoop::IsGameOver(uint8_t& outWinnerId)
{
    int aliveCount = 0;
    outWinnerId = INVALID_PLAYER_ID;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        CPlayer* player = m_playerById[i];

        if (!player || player->IsDead() || player->IsGhost())
        {
            continue;
        }

        ++aliveCount;
        outWinnerId = (uint8_t)i;
    }

    if (aliveCount > 1)
    {
        return false;
    }

    if (aliveCount == 0)
    {
        outWinnerId = INVALID_PLAYER_ID;
    }

    return true;
}

void CGameLoop::SendResult(uint8_t winnerId)
{
    SC_GameResult packet = {};
    packet.header.size = sizeof(SC_GameResult);
    packet.header.type = (uint8_t)EPacketType::SC_GameResult;
    packet.winnerPlayerId = winnerId;

    m_server->SendToAll((const char*)&packet, sizeof(packet));

    printf("게임 결과 전송 (승자 %d)\n", winnerId);
}