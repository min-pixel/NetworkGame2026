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

struct InputSlot
{
    bool active = false;
    uint32_t lastSequence = 0;
    CS_Input input = {};
    sockaddr_in addr = {};
    bool hasAddr = false;
};

class CGameServer
{
public:
    ~CGameServer();

    bool Start();

    void AcceptLoop();
    static DWORD WINAPI TcpCommThread(LPVOID param);   
    void TcpCommLoop(SOCKET sock);

    uint8_t TryRegisterSeat(SOCKET sock, ERejectReason& outReason); 
    bool SendPacket(CClientSession& s, const char* data, int len); 

    void SendConnectResult(SOCKET sock, uint8_t playerId, ERejectReason reason);

    static DWORD WINAPI UdpRecvThread(LPVOID param);
    void UdpRecvLoop();

private:
    SOCKET m_listenSock = INVALID_SOCKET;
    SOCKET m_udpSock = INVALID_SOCKET;

    CClientSession m_seats[MAX_PLAYERS];
    bool m_inGame = false;
    CRITICAL_SECTION m_seatLock;

    InputSlot m_inputs[MAX_PLAYERS];
    CRITICAL_SECTION m_inputLock;
    HANDLE m_udpRecvThread = NULL;

};
