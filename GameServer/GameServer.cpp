#include "GameServer.h"
#include <cstdio>

CGameServer::~CGameServer()
{
    closesocket(m_listenSock);
    closesocket(m_udpSock);
    WSACleanup();
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
