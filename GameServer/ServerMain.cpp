#include "GameServer.h"

int main()
{
    CGameServer server;

    if (!server.Start())
    {
        return 1;
    }

    server.AcceptLoop();
    return 0;
}
