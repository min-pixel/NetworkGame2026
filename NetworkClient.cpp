#include "stdafx.h"
#include "NetworkClient.h"
#include "NetUtil.h"
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

CNetworkClient::CNetworkClient()
{
    InitializeCriticalSection(&m_eventCs);
}

CNetworkClient::~CNetworkClient()
{
    bool connected = (m_tcpSock != INVALID_SOCKET);

    if (connected)
    {
        shutdown(m_tcpSock, SD_BOTH);
        closesocket(m_tcpSock);
        m_tcpSock = INVALID_SOCKET;
    }

    if (m_udpSock != INVALID_SOCKET)
    {
        closesocket(m_udpSock);
        m_udpSock = INVALID_SOCKET;
    }

    if (m_tcpThread != NULL)
    {
        WaitForSingleObject(m_tcpThread, INFINITE);
        CloseHandle(m_tcpThread);
    }

    DeleteCriticalSection(&m_eventCs);

    if (connected)
    {
        WSACleanup();
    }
}


bool CNetworkClient::Connect(const char* serverIp)
{
    WSADATA wsaData;

    if (m_tcpSock != INVALID_SOCKET)
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
    m_serverUdpAddr = addr;
    m_serverUdpAddr.sin_port = htons(SERVER_UDP_PORT);
    return true;
}


bool CNetworkClient::StartReceive()
{
    if (m_tcpSock == INVALID_SOCKET)
    {
        return false;
    }

    if (m_udpSock != INVALID_SOCKET || m_tcpThread != NULL)
    {
        return false;
    }

    m_udpSock = socket(AF_INET, SOCK_DGRAM, 0);

    if (m_udpSock == INVALID_SOCKET)
    {
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
        closesocket(m_udpSock);
        m_udpSock = INVALID_SOCKET;
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

void CNetworkClient::TcpRecvLoop()
{
    char buf[1024];

    while (1)
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
    if (m_udpSock == INVALID_SOCKET)
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