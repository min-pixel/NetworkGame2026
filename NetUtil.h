#pragma once
#include <winsock2.h>
#include <string.h>
#include "Protocol.h"
#pragma comment(lib, "ws2_32.lib")

inline bool SendAll(SOCKET sock, const char* data, int len)
{
    int total = 0;
    int n;

    while (total < len)
    {
        n = send(sock, data + total, len - total, 0);
        if (n == SOCKET_ERROR || n == 0)
        {
            return false;
        }
        total = total + n;
    }
    return true;
}

inline bool RecvPacket(SOCKET sock, char* buf, int bufSize)
{
    PacketHeader header;
    int total = 0;
    int n;
    int packetSize = 0;

    if (buf == NULL || bufSize < (int)sizeof(PacketHeader))
    {
        return false;
    }

    while (total < (int)sizeof(PacketHeader))
    {
        n = recv(sock, buf + total,
            (int)sizeof(PacketHeader) - total, 0);
        if (n <= 0)
        {
            return false;
        }
        total = total + n;
    }

    memcpy(&header, buf, sizeof(PacketHeader));

    if (header.size < sizeof(PacketHeader) || header.size > bufSize)
        return false;

    switch ((EPacketType)header.type)
    {
    case EPacketType::CS_Ready:
        packetSize = sizeof(CS_Ready);
        break;

    case EPacketType::SC_ConnectResult:
        packetSize = sizeof(SC_ConnectResult);
        break;

    case EPacketType::SC_LobbyState:
        packetSize = sizeof(SC_LobbyState);
        break;

    case EPacketType::SC_GameStart:
        packetSize = sizeof(SC_GameStart);
        break;

    case EPacketType::SC_Event:
        packetSize = sizeof(SC_Event);
        break;

    case EPacketType::SC_GameResult:
        packetSize = sizeof(SC_GameResult);
        break;

    default:
        return false;
    }

    if (header.size != packetSize)
    {
        return false;
    }

    while (total < header.size)
    {
        n = recv(sock, buf + total, header.size - total, 0);

        if (n <= 0)
        {
            return false;
        }
        total = total + n;
    }
    return true;
}
