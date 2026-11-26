#include "stdafx.h"
#include "NetworkClient.h"
#include "NetUtil.h"
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

CNetworkClient::CNetworkClient()
{
    InitializeCriticalSection(&m_eventCs);
    InitializeCriticalSection(&m_stateCs);
}

CNetworkClient::~CNetworkClient()
{
    Disconnect();
    DeleteCriticalSection(&m_eventCs);
    DeleteCriticalSection(&m_stateCs);
}

void CNetworkClient::Disconnect()
{
    m_stopRequested.store(true);

    if (m_tcpSock != INVALID_SOCKET)
    {
        shutdown(m_tcpSock, SD_BOTH);
    }

    if (m_tcpThread != NULL)
    {
        WaitForSingleObject(m_tcpThread, INFINITE);
        CloseHandle(m_tcpThread);
        m_tcpThread = NULL;
    }

    if (m_udpThread != NULL)
    {
        WaitForSingleObject(m_udpThread, INFINITE);
        CloseHandle(m_udpThread);
        m_udpThread = NULL;
    }

    if (m_tcpSock != INVALID_SOCKET)
    {
        closesocket(m_tcpSock);
        m_tcpSock = INVALID_SOCKET;
    }

    if (m_udpSock != INVALID_SOCKET)
    {
        closesocket(m_udpSock);
        m_udpSock = INVALID_SOCKET;
    }

    EnterCriticalSection(&m_eventCs);
    while (!m_eventQueue.empty())
    {
        m_eventQueue.pop();
    }
    LeaveCriticalSection(&m_eventCs);

    EnterCriticalSection(&m_stateCs);
    m_latestStates = {};
    for (int i = 0; i < 3; ++i)
    {
        m_lastTick[i] = 0;
        m_hasTick[i] = false;
    }
    LeaveCriticalSection(&m_stateCs);

    m_serverUdpAddr = {};
    m_inputSequence = 0;
    m_myPlayerId = INVALID_PLAYER_ID;

    if (m_wsaStarted)
    {
        WSACleanup();
        m_wsaStarted = false;
    }
}

bool CNetworkClient::Connect(const char* serverIp)
{
    WSADATA wsaData;

    if (m_tcpSock != INVALID_SOCKET || m_wsaStarted)
    {
        return false;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
    {
        WSACleanup();
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_TCP_PORT);

    if (inet_pton(AF_INET, serverIp, &addr.sin_addr) != 1)
    {
        closesocket(sock);
        WSACleanup();
        return false;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        WSACleanup();
        return false;
    }
    m_tcpSock = sock;
    m_wsaStarted = true;
    m_serverUdpAddr = addr;
    m_serverUdpAddr.sin_port = htons(SERVER_UDP_PORT);
    return true;
}



bool CNetworkClient::StartReceive()
{
    if (m_tcpSock == INVALID_SOCKET || !m_wsaStarted)
    {
        return false;
    }

    if (m_udpSock != INVALID_SOCKET ||
        m_tcpThread != NULL ||
        m_udpThread != NULL)
    {
        return false;
    }

    m_udpSock = socket(AF_INET, SOCK_DGRAM, 0);

    if (m_udpSock == INVALID_SOCKET)
    {
        Disconnect();
        return false;
    }

    sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(0);

    if (bind(m_udpSock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
    {
        Disconnect();
        return false;
    }

    DWORD receiveTimeout = 200;
    if (setsockopt(m_udpSock, SOL_SOCKET, SO_RCVTIMEO,
        (const char*)&receiveTimeout, sizeof(receiveTimeout)) == SOCKET_ERROR)
    {
        Disconnect();
        return false;
    }

    m_stopRequested.store(false);

    m_udpThread = CreateThread(
        NULL,
        0,
        UdpRecvThread,
        this,
        0,
        NULL
    );

    if (m_udpThread == NULL)
    {
        Disconnect();
        return false;
    }

    m_tcpThread = CreateThread(
        NULL,
        0,
        TcpRecvThread,
        this,
        0,
        NULL
    );

    if (m_tcpThread == NULL)
    {
        Disconnect();
        return false;
    }

    return true;
}

DWORD WINAPI CNetworkClient::TcpRecvThread(LPVOID param)
{
    CNetworkClient* client = (CNetworkClient*)param;

    client->TcpRecvLoop();

    return 0;
}


DWORD WINAPI CNetworkClient::UdpRecvThread(LPVOID param)
{
    CNetworkClient* client = (CNetworkClient*)param;

    client->UdpRecvLoop();

    return 0;
}


void CNetworkClient::UdpRecvLoop()
{
    char buf[1024];

    while (!m_stopRequested.load())
    {
        sockaddr_in senderAddr = {};
        int senderLen = sizeof(senderAddr);

        int received = recvfrom(
            m_udpSock,
            buf,
            sizeof(buf),
            0,
            (sockaddr*)&senderAddr,
            &senderLen
        );

        if (received == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT || error == WSAEMSGSIZE)
            {
                continue;
            }
            break;
        }

        if (received < STATE_PACKET_HEAD_SIZE)
        {
            continue;
        }

        if (senderAddr.sin_addr.s_addr != m_serverUdpAddr.sin_addr.s_addr ||
            senderAddr.sin_port != m_serverUdpAddr.sin_port)
        {
            continue;
        }

        PacketHeader header = {};
        memcpy(&header, buf, sizeof(PacketHeader));

        if (header.size != received)
        {
            continue;
        }

        if (header.type == (uint8_t)EPacketType::SC_PlayerStates)
        {
            if (received > (int)sizeof(SC_PlayerStates))
            {
                continue;
            }

            SC_PlayerStates states = {};
            memcpy(&states, buf, received);

            if (states.count > MAX_PLAYERS)
            {
                continue;
            }

            int expectedSize = STATE_PACKET_HEAD_SIZE +
                states.count * sizeof(PlayerStateInfo);

            if (received != expectedSize)
            {
                continue;
            }

            EnterCriticalSection(&m_stateCs);

            if (!m_hasTick[0] || states.tick > m_lastTick[0])
            {
                m_latestStates.playerStates = states;
                m_lastTick[0] = states.tick;
                m_hasTick[0] = true;
            }

            LeaveCriticalSection(&m_stateCs);
        }
        else if (header.type == (uint8_t)EPacketType::SC_EnemyStates)
        {
            if (received > (int)sizeof(SC_EnemyStates))
            {
                continue;
            }

            SC_EnemyStates states = {};
            memcpy(&states, buf, received);

            if (states.count > MAX_ENEMIES)
            {
                continue;
            }

            int expectedSize = STATE_PACKET_HEAD_SIZE +
                states.count * sizeof(EnemyStateInfo);

            if (received != expectedSize)
            {
                continue;
            }

            EnterCriticalSection(&m_stateCs);

            if (!m_hasTick[1] || states.tick > m_lastTick[1])
            {
                m_latestStates.enemyStates = states;
                m_lastTick[1] = states.tick;
                m_hasTick[1] = true;
            }

            LeaveCriticalSection(&m_stateCs);
        }
        else if (header.type == (uint8_t)EPacketType::SC_BulletStates)
        {
            if (received > (int)sizeof(SC_BulletStates))
            {
                continue;
            }

            SC_BulletStates states = {};
            memcpy(&states, buf, received);

            if (states.count > MAX_BULLETS)
            {
                continue;
            }

            int expectedSize = STATE_PACKET_HEAD_SIZE +
                states.count * sizeof(BulletStateInfo);

            if (received != expectedSize)
            {
                continue;
            }

            EnterCriticalSection(&m_stateCs);

            if (!m_hasTick[2] || states.tick > m_lastTick[2])
            {
                m_latestStates.bulletStates = states;
                m_lastTick[2] = states.tick;
                m_hasTick[2] = true;
            }

            LeaveCriticalSection(&m_stateCs);
        }
    }
}

void CNetworkClient::TcpRecvLoop()
{
    char buf[1024];

    while (!m_stopRequested.load())
    {
        if (RecvPacket(m_tcpSock, buf, sizeof(buf)) == false)
        {
            break;
        }

        PacketHeader header;
        memcpy(&header, buf, sizeof(PacketHeader));

        NetEvent event = {};
        event.type = (EPacketType)header.type;

        switch (event.type)
        {
        case EPacketType::SC_ConnectResult:
            memcpy(&event.connectResult, buf, sizeof(SC_ConnectResult));
            break;

        case EPacketType::SC_LobbyState:
            memcpy(&event.lobbyState, buf, sizeof(SC_LobbyState));
            break;

        case EPacketType::SC_GameStart:
            memcpy(&event.gameStart, buf, sizeof(SC_GameStart));
            break;

        case EPacketType::SC_Event:
            memcpy(&event.gameEvent, buf, sizeof(SC_Event));
            break;

        case EPacketType::SC_GameResult:
            memcpy(&event.gameResult, buf, sizeof(SC_GameResult));
            break;

        default:
            continue;
        }

        EnterCriticalSection(&m_eventCs);
        m_eventQueue.push(event);
        LeaveCriticalSection(&m_eventCs);
    }
}

void CNetworkClient::PopEvents(std::vector<NetEvent>& events)
{
    EnterCriticalSection(&m_eventCs);

    while (!m_eventQueue.empty())
    {
        events.push_back(m_eventQueue.front());
        m_eventQueue.pop();
    }

    LeaveCriticalSection(&m_eventCs);
}

void CNetworkClient::CopyLatestStates(LatestStates& states)
{
    EnterCriticalSection(&m_stateCs);

    states = m_latestStates;

    LeaveCriticalSection(&m_stateCs);
}

void CNetworkClient::ClearLatestStates()
{
    EnterCriticalSection(&m_stateCs);
    m_latestStates = {};
    LeaveCriticalSection(&m_stateCs);
}

bool CNetworkClient::SendReady()
{
    if (m_stopRequested.load() || m_tcpSock == INVALID_SOCKET ||
        m_myPlayerId == INVALID_PLAYER_ID)
    {
        return false;
    }

    CS_Ready packet = {};
    packet.header.size = sizeof(packet);
    packet.header.type = (uint8_t)EPacketType::CS_Ready;

    return SendAll(m_tcpSock, (const char*)&packet, sizeof(packet));
}

void CNetworkClient::SetMyPlayerId(uint8_t playerId)
{
    m_myPlayerId = playerId;
}

uint8_t CNetworkClient::GetMyPlayerId() const
{
    return m_myPlayerId;
}

void CNetworkClient::SendInput(int8_t moveForward, int8_t moveRight,
    float cameraYaw, uint8_t fire, uint8_t shieldPressCount)
{
    if (m_stopRequested.load() || m_udpSock == INVALID_SOCKET)
    {
        return;
    }

    if (m_myPlayerId == INVALID_PLAYER_ID)
    {
        return;
    }

    CS_Input packet = {};

    packet.header.size = sizeof(CS_Input);
    packet.header.type = (uint8_t)EPacketType::CS_Input;

    packet.playerId = m_myPlayerId;
    packet.sequence = m_inputSequence++;

    packet.moveForward = moveForward;
    packet.moveRight = moveRight;
    packet.cameraYaw = cameraYaw;
    packet.fire = fire;
    packet.shieldPressCount = shieldPressCount;

    sendto(
        m_udpSock,
        (const char*)&packet,
        sizeof(packet),
        0,
        (sockaddr*)&m_serverUdpAddr,
        sizeof(m_serverUdpAddr)
    );
}