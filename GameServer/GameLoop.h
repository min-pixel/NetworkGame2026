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

private:
    CGameServer* m_server;
    CTerrainGameMap* m_map = nullptr;
    CGameWorld m_world;
    CPlayer* m_playerById[MAX_PLAYERS] = {};

    CGameTimer m_timer;
    uint8_t m_lastShieldCount[MAX_PLAYERS] = {};

};

