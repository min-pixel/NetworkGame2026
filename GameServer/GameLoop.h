#pragma once

#include "stdafx.h"
#include "Protocol.h"
#include "GameWorld.h"
#include "Timer.h"

class CGameServer;
class CTerrainGameMap;
class CPlayer;

class CGameLoop
{
public:
    explicit CGameLoop(CGameServer* server) : m_server(server) {}

    static DWORD WINAPI GameLoopThread(LPVOID param);
    bool PrepareGame();


    void WaitNextTick();
    void UpdateWorld(const CS_Input inputs[MAX_PLAYERS], std::vector<SC_Event>& outEvents);
    PlayerInput ToPlayerInput(const CS_Input& input);

    void Run();
    void RunTick();
    void SendStates();
    int FillPlayerStates(SC_PlayerStates& out);

    int FillEnemyStates(SC_EnemyStates& out);


    int FillBulletStates(SC_BulletStates& out);

    void SendEvents(const std::vector<SC_Event>& events);

    bool IsGameOver(uint8_t& outWinnerId);

    void SendResult(uint8_t winnerId);
private:
    CGameServer* m_server;
    CTerrainGameMap* m_map = nullptr;
    CGameWorld m_world;
    CPlayer* m_playerById[MAX_PLAYERS] = {};

    CGameTimer m_timer;
    uint8_t m_lastShieldCount[MAX_PLAYERS] = {};

    uint32_t m_tick = 0;

};

