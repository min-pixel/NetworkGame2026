#pragma once

#include "Protocol.h"
#include <winsock2.h>
#include <windows.h>
#include <queue>
#include <vector>
#include <atomic>

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

    sockaddr_in m_serverUdpAddr = {};
    uint32_t m_inputSequence = 0;

    HANDLE m_tcpThread = NULL;
    HANDLE m_udpThread = NULL;

    bool m_wsaStarted = false;
    std::atomic<bool> m_stopRequested{ true };

    CRITICAL_SECTION m_eventCs;
    CRITICAL_SECTION m_stateCs;

    std::queue<NetEvent> m_eventQueue;
    LatestStates m_latestStates = {};

    uint32_t m_lastTick[3] = {};
    bool m_hasTick[3] = {};

    static DWORD WINAPI TcpRecvThread(LPVOID param);
    void TcpRecvLoop();

    static DWORD WINAPI UdpRecvThread(LPVOID param);
    void UdpRecvLoop();

    uint8_t m_myPlayerId = INVALID_PLAYER_ID;

public:
    CNetworkClient();
    ~CNetworkClient();
    void PopEvents(std::vector<NetEvent>& events);
    void CopyLatestStates(LatestStates& states);
    void ClearLatestStates();

    bool Connect(const char* serverIp);
    bool StartReceive();
    void Disconnect();

    bool SendReady();

    void SendInput(int8_t moveForward, int8_t moveRight, float cameraYaw, uint8_t fire, uint8_t shieldPressCount);

    void SetMyPlayerId(uint8_t playerId);
    uint8_t GetMyPlayerId() const;
};