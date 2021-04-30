#ifndef _BP35C0_J1_H_
#define _BP35C0_J1_H_
#include "WisunManager.h"

// Unique Code
#define UNQ_REQ 0xd0, 0xea, 0x83, 0xfc
#define UNQ_RES (0xd0f9ee5d)
#define UNQ_INF (0xd0f9ee5d)
// Command Code
#define CMD_GET_STATUS (0x0001)
#define CMD_GET_UDP (0x0007)
#define CMD_GET_IP (0x0009)
#define CMD_GET_MAC (0x000e)
#define CMD_GET_CONNECT (0x0011)
#define CMD_GET_TERMINAL (0x0100)
#define CMD_GET_NEIGHBOR_DIS (0x0102)
#define CMD_GET_INITIAL (0x0107)
#define CMD_GET_UART (0x010b)
#define CMD_INITIALIZE 0x00, 0x5f
#define CMD_SET_NEIGHBOR_DIS (0x0101)
#define CMD_SET_UART (0x010a)
#define CMD_UDP_OPEN 0x00, 0x05
#define CMD_UDP_CLOSE 0x00, 0x06
#define CMD_SEND_DATA 0x00, 0x08
#define CMD_ACTIVE_SCAN 0x00, 0x51
#define CMD_SEND_PING 0x00, 0xd1
#define CMD_ED_SCAN 0x00, 0xdb
#define CMD_GET_VERSION 0x00, 0x6b
#define CMD_HW_RESET 0x00, 0xd9
#define CMD_WRITE_MODE 0x00, 0xf0
#define CMD_B_ROUTE_GET_ENCRYPTION_KEY 0x00, 0x59
#define CMD_B_ROUTE_GET_PANID 0x00, 0x5e
#define CMD_B_ROUTE_SET_PANA_INFO 0x00, 0x54
#define CMD_B_ROUTE_START 0x00, 0x53
#define CMD_B_ROUTE_PANA_START 0x00, 0x56
#define CMD_B_ROUTE_PANA_END 0x00, 0x57
#define CMD_B_ROUTE_END 0x00, 0x58
#define CMD_B_ROUTE_PANA_RE_AUTH 0x00, 0xd2
// Response Code
#define RES_GET_STATUS (0x2001)
#define RES_GET_UDP (0x2007)
#define RES_GET_IP (0x2009)
#define RES_GET_MAC (0x200e)
#define RES_GET_CONNECT (0x2011)
#define RES_GET_TERMINAL (0x2100)
#define RES_GET_NEIGHBOR_DIS (0x2102)
#define RES_GET_INITIAL (0x2107)
#define RES_GET_UART (0x210b)
#define RES_INITIALIZE (0x205f)
#define RES_SET_NEIGHBOR_DIS (0x2101)
#define RES_SET_UART (0x210a)
#define RES_UDP_OPEN (0x2005)
#define RES_UDP_CLOSE (0x2006)
#define RES_SEND_DATA (0x2008)
#define RES_ACTIVE_SCAN (0x2051)
#define RES_SEND_PING (0x20d1)
#define RES_ED_SCAN (0x20db)
#define RES_GET_VERSION (0x206b)
#define RES_WRITE_MODE (0x20f0)
#define RES_B_ROUTE_GET_ENCRYPTION_KEY (0x2059)
#define RES_B_ROUTE_GET_PANID (0x205e)
#define RES_B_ROUTE_SET_PANA_INFO (0x2054)
#define RES_B_ROUTE_START (0x2053)
#define RES_B_ROUTE_PANA_START (0x2056)
#define RES_B_ROUTE_PANA_END (0x2057)
#define RES_B_ROUTE_END (0x2058)
#define RES_B_ROUTE_PANA_RE_AUTH (0x20d2)
// Information Code
#define INF_ACTIVE_SCAN (0x4051)
#define INF_SEND_PING (0x60d1)
#define INF_RECV_DATA (0x6018)
#define INF_BOOTED (0x6019)
#define INF_CHANGE_CONNECT_STATE (0x601a)
#define INF_PANA_AUTH (0x6028)
#define INF_RECV_ERROR (0x6038)

#define DEFAULT_CH 4
class BP35C0_J1 : public WisunManager
{
private:
    TaskHandle_t _connectTaskHandle;
    uint8_t _ipv6_addr[16];
    uint16_t _pan_id;
    uint8_t _ch = DEFAULT_CH;
    uint8_t _mac[8];
    int8_t _rssi;

public:
    BP35C0_J1();
    ~BP35C0_J1();
    virtual void connect();
    virtual void disconnect();
    virtual int read();
    virtual void begin();
    virtual void udpSend(const uint8_t *frame, size_t size);

    //
    static void connectTask(void *arm);
    static void receiveTask(void *arm);
};
#endif