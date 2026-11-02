#pragma once
#include <winsock2.h>
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
