#pragma once

#include "Protocol.h"

struct NetEvent
{
    EPacketType type;

    SC_ConnectResult connectResult;
    SC_LobbyState lobbyState;
    SC_GameStart gameStart;
    SC_Event gameEvent;
    SC_GameResult gameResult;
};

struct LatestStates
{
    SC_PlayerStates playerStates = {};
    SC_EnemyStates enemyStates = {};
    SC_BulletStates bulletStates = {};
};

