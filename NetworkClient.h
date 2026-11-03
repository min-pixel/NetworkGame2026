#pragma once

#include "Protocol.h"
#include <winsock2.h>

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

class CNetworkClient
{
private:
    SOCKET m_tcpSock = INVALID_SOCKET;

public:
    bool Connect(const char* serverIp);
};

