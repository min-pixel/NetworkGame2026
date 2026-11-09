#pragma once

#include "stdafx.h"
#include "Protocol.h"
#include "GameWorld.h"

class CGameServer;
class CTerrainGameMap;
class CPlayer;

class CGameLoop
{
public:
    explicit CGameLoop(CGameServer* server) : m_server(server) {}

    static DWORD WINAPI GameLoopThread(LPVOID param);
    bool PrepareGame();

private:
    CGameServer* m_server;
    CTerrainGameMap* m_map = nullptr;
    CGameWorld m_world;
    CPlayer* m_playerById[MAX_PLAYERS] = {};




};

