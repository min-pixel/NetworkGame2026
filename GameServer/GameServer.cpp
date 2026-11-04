#include "GameServer.h"
#include <cstdio>

struct TcpThreadParam
{
    CGameServer* server;
    SOCKET sock;
};

CGameServer::CGameServer()
{
    InitializeCriticalSection(&m_seatLock);

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        InitializeCriticalSection(&m_seats[i].sendLock);
    }
}

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

}

bool CGameServer::Start()
{
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
    //SendConnectResult(sock, playerId, reason);

    if (playerId == INVALID_PLAYER_ID)
    {
        closesocket(sock);
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