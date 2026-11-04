#pragma once

#include "Protocol.h"
#include <winsock2.h>
#include <windows.h>
#include <queue>
#include <vector>

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
    SOCKET m_udpSock = INVALID_SOCKET;

    HANDLE m_tcpThread = NULL;
    CRITICAL_SECTION m_eventCs;
    std::queue<NetEvent> m_eventQueue;

    static DWORD WINAPI TcpRecvThread(LPVOID param);
    void TcpRecvLoop();

public:
    CNetworkClient();
    ~CNetworkClient();
    void PopEvents(std::vector<NetEvent>& events);

    bool Connect(const char* serverIp);
    bool StartReceive();
};