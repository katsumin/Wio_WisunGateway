#ifndef _WISUN_MANAGER_H_
#define _WISUN_MANAGER_H_
#include <Arduino.h>
#include "DataStore.h"
#include "RequestResponse.h"
#include <Seeed_Arduino_FreeRTOS.h>
#include "config.h"

typedef enum
{
    DIS_CONNECT,    // 切断
    CONNECT,        // 接続
    CONNECTING,     // 接続中
    DIS_CONNECTING, // 切断中
} WISUN_STATE;

/**
 * Wi-SUNマネージャ
 */
class WisunManager
{
private:
    DataStore *_dataStore;
    WISUN_STATE _state = DIS_CONNECT;

protected:
    uint8_t _rbuf[BUF_SIZE];

public:
    WisunManager();
    virtual ~WisunManager();
    virtual void connect() {}
    virtual void disconnect() {}
    virtual int read() { return 0; }
    virtual void begin();
    void stop();
    void update();
    void get(RequestResponse &rr);
    inline void setDataStore(DataStore *ds)
    {
        _dataStore = ds;
        ds->setWisunManager(this);
    }
    inline DataStore *getDataStore() { return _dataStore; }
    virtual void udpSend(const uint8_t *frame, size_t size) {}
    virtual inline WISUN_STATE getState() { return _state; }
    virtual inline void setState(WISUN_STATE state) { _state = state; }

    //
    static void sendSerial(const uint8_t *buffer, size_t size);
    static void sendTask(void *arm);
};

#endif