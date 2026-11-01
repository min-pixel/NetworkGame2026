#include <winsock2.h>
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

class CGameServer
{
public:
    ~CGameServer();

    bool Start();

private:
    SOCKET m_listenSock = INVALID_SOCKET;
    SOCKET m_udpSock = INVALID_SOCKET;
};
