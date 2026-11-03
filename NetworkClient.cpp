#include "stdafx.h"
#include "NetworkClient.h"
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

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
    return true;
}