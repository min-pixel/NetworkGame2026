#include "NetUtil.h"

#pragma comment(lib, "ws2_32.lib")

struct CClientSession
{
    bool used = false;
    bool ready = false;
    SOCKET sock = INVALID_SOCKET;
    HANDLE thread;
    CRITICAL_SECTION sendLock;
};

class CGameServer
{
public:
    ~CGameServer();

    bool Start();

    //20261104
    void AcceptLoop();
    static DWORD WINAPI TcpCommThread(LPVOID param);   
    void TcpCommLoop(SOCKET sock);

    //20261104-2
    uint8_t TryRegisterSeat(SOCKET sock, ERejectReason& outReason); 
    bool SendPacket(CClientSession& s, const char* data, int len); 

    //20261105
    void SendConnectResult(SOCKET sock, uint8_t playerId, ERejectReason reason);

private:
    SOCKET m_listenSock = INVALID_SOCKET;
    SOCKET m_udpSock = INVALID_SOCKET;

    //20261104
    CClientSession m_seats[MAX_PLAYERS];
    bool m_inGame = false;
    CRITICAL_SECTION m_seatLock;
};
