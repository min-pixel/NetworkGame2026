#include "GameServer.h"
#include <cstdio>

int main()
{
    CGameServer server;

    if (!server.Start())
    {
        return 1;
    }

    getchar();
    return 0;
}
