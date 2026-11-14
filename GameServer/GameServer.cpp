#include "GameServer.h"
#include <cstdio>

struct TcpThreadParam
{
    CGameServer* server;
    SOCKET sock;
};

CGameServer::~CGameServer()
{
    closesocket(m_listenSock);
    closesocket(m_udpSock);
    WSACleanup();

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        DeleteCriticalSection(&m_seats[i].sendLock);
    }

    DeleteCriticalSection(&m_seatLock);
    DeleteCriticalSection(&m_inputLock);
}

bool CGameServer::Start()
{
    InitializeCriticalSection(&m_seatLock);
    InitializeCriticalSection(&m_inputLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        InitializeCriticalSection(&m_seats[i].sendLock);
    }

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_port = htons(SERVER_TCP_PORT);

    if (bind(m_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || listen(m_listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("TCP 실패 (%d)\n", WSAGetLastError());
        return false;
    }

    m_udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_port = htons(SERVER_UDP_PORT);

    if (bind(m_udpSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("UDP 실패 (%d)\n", WSAGetLastError());
        return false;
    }

    m_udpRecvThread = CreateThread(NULL, 0, UdpRecvThread, this, 0, NULL);

    m_gameLoopThread = CreateThread(NULL, 0, CGameLoop::GameLoopThread, &m_gameLoop, 0, NULL);

    printf("서버 시작\n");
    return true;
}

void CGameServer::AcceptLoop()
{
    while (true)
    {
        SOCKET sock = accept(m_listenSock, NULL, NULL);

        if (sock == INVALID_SOCKET)
        {
            printf("accept 실패 (%d)\n", WSAGetLastError());
            continue;
        }

        TcpThreadParam* param = new TcpThreadParam{ this, sock };
        CloseHandle(CreateThread(NULL, 0, TcpCommThread, param, 0, NULL));
    }
}

DWORD WINAPI CGameServer::TcpCommThread(LPVOID param)
{
    TcpThreadParam* p = (TcpThreadParam*)param;
    CGameServer* server = p->server;
    SOCKET sock = p->sock;
    delete p;

    server->TcpCommLoop(sock);
    return 0;
}

uint8_t CGameServer::TryRegisterSeat(SOCKET sock, ERejectReason& outReason)
{
    uint8_t playerId = INVALID_PLAYER_ID;
    outReason = ERejectReason::ServerFull;

    EnterCriticalSection(&m_seatLock);

    if (m_inGame)
    {
        outReason = ERejectReason::AlreadyStarted;
    }
    else
    {
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!m_seats[i].used)
            {
                m_seats[i].used = true;
                m_seats[i].ready = false;

                EnterCriticalSection(&m_inputLock);
                m_inputs[i] = InputSlot();
                m_inputs[i].active = true;
                LeaveCriticalSection(&m_inputLock);

                EnterCriticalSection(&m_seats[i].sendLock);
                m_seats[i].sock = sock;
                LeaveCriticalSection(&m_seats[i].sendLock);

                playerId = (uint8_t)i;
                outReason = ERejectReason::None;
                break;
            }
        }
    }

    LeaveCriticalSection(&m_seatLock);
    return playerId;
}

bool CGameServer::SendPacket(CClientSession& s, const char* data, int len)
{
    EnterCriticalSection(&s.sendLock);
    bool ok = SendAll(s.sock, data, len);
    LeaveCriticalSection(&s.sendLock);
    return ok;
}


void CGameServer::TcpCommLoop(SOCKET sock)
{
    ERejectReason reason;
    uint8_t playerId = TryRegisterSeat(sock, reason);
    SendConnectResult(sock, playerId, reason);

    if (playerId == INVALID_PLAYER_ID)
    {
        return;
    }

    printf("%d번 접속\n", playerId);

    char buf[sizeof(CS_Ready)];

    while (RecvPacket(sock, buf, sizeof(buf)))
    {
    }

    printf("%d번 끊김\n", playerId);
    closesocket(sock);
}

void CGameServer::SendConnectResult(SOCKET sock, uint8_t playerId, ERejectReason reason)
{
    SC_ConnectResult packet = {};
    packet.header.size = sizeof(packet);
    packet.header.type = (uint8_t)EPacketType::SC_ConnectResult;
    packet.accepted = (playerId != INVALID_PLAYER_ID);
    packet.playerId = playerId;
    packet.rejectReason = (uint8_t)reason;

    if (playerId == INVALID_PLAYER_ID)
    {
        SendAll(sock, (const char*)&packet, sizeof(packet));
        closesocket(sock);
    }
    else
    {
        SendPacket(m_seats[playerId], (const char*)&packet, sizeof(packet));
    }
}


DWORD WINAPI CGameServer::UdpRecvThread(LPVOID param)
{
    CGameServer* server = (CGameServer*)param;
    server->UdpRecvLoop();
    return 0;
}

void CGameServer::UdpRecvLoop()
{
    CS_Input packet;
    sockaddr_in from;
    int fromLen;

    while (true)
    {
        fromLen = sizeof(from);
        int n = recvfrom(m_udpSock, (char*)&packet, sizeof(packet), 0, (sockaddr*)&from, &fromLen);

        if (n == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            if (error == WSAECONNRESET || error == WSAEMSGSIZE)
            {
                continue;
            }

            break;
        }

        if (n != sizeof(CS_Input) || packet.header.size != sizeof(CS_Input) || packet.header.type != (uint8_t)EPacketType::CS_Input || packet.playerId >= MAX_PLAYERS)
        {
            continue;
        }

        EnterCriticalSection(&m_inputLock);

        InputSlot& slot = m_inputs[packet.playerId];

        if (slot.active && packet.sequence > slot.lastSequence)
        {
            if (!slot.hasAddr)
            {
                slot.addr = from;
                slot.hasAddr = true;
                //printf("%d번 UDP 주소 등록\n", packet.playerId);
                
            }

            if (slot.addr.sin_addr.s_addr == from.sin_addr.s_addr && slot.addr.sin_port == from.sin_port)
            {
                slot.input = packet;
                slot.lastSequence = packet.sequence;
                if (packet.sequence % 30 == 0) printf("%d번 입력 순번 %u 앞뒤 %d 좌우 %d 카메라 %.1f\n", packet.playerId, packet.sequence, packet.moveForward, packet.moveRight, packet.cameraYaw);
            }
        }

        LeaveCriticalSection(&m_inputLock);
    }
}

void CGameServer::CopyInputs(CS_Input outInputs[MAX_PLAYERS])
{
    EnterCriticalSection(&m_inputLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        outInputs[i] = m_inputs[i].input;
    }

    LeaveCriticalSection(&m_inputLock);
}

void CGameServer::CopyUdpAddrs(sockaddr_in outAddrs[MAX_PLAYERS], bool outHasAddr[MAX_PLAYERS])
{
    EnterCriticalSection(&m_inputLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        outAddrs[i] = m_inputs[i].addr;
        outHasAddr[i] = m_inputs[i].hasAddr;
    }

    LeaveCriticalSection(&m_inputLock);
}

void CGameServer::SendToAll(const char* data, int len)
{
    bool used[MAX_PLAYERS];

    EnterCriticalSection(&m_seatLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        used[i] = m_seats[i].used;
    }

    LeaveCriticalSection(&m_seatLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (used[i])
        {
            SendPacket(m_seats[i], data, len);
        }
    }
}

void CGameServer::GetParticipants(bool outJoined[MAX_PLAYERS])
{
    EnterCriticalSection(&m_seatLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        outJoined[i] = m_seats[i].used;
    }

    LeaveCriticalSection(&m_seatLock);
}

bool CGameServer::TryStartGame()
{
    bool start = false;

    EnterCriticalSection(&m_seatLock);

    if (!m_inGame)
    {
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (m_seats[i].used)
            {
                start = true;
            }
        }

        m_inGame = start;
    }

    LeaveCriticalSection(&m_seatLock);
    return start;
}