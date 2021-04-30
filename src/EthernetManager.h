#ifndef _ETHER_MANAGER_H_
#define _ETHER_MANAGER_H_
#include "EL.h"
#include "DataStore.h"
#include "RequestResponse.h"
#include "debug.h"

// ユニキャスト用ソケットを使うようにELクラスを拡張
class EL2 : public EL
{
private:
    UDP *_uniUdp;

public:
    EL2(UDP &udp, UDP &uniUdp, byte eoj0, byte eoj1, byte eoj2) : EL(udp, eoj0, eoj1, eoj2)
    {
        _uniUdp = &uniUdp;
    }

    EL2(UDP &udp, UDP &uniUdp, byte eojs[][3], int count) : EL(udp, eojs, count)
    {
        _uniUdp = &uniUdp;
    }

    void begin(void)
    {
        Serial.println("EL2::begin");
        // udp
        if (_uniUdp->begin(EL_PORT + 1))
        {
            Serial.println("EL.udp.begin successful.");
        }
        else
        {
            Serial.println("Reseiver udp.begin failed."); // localPort
        }

        if (_udp->beginMulticast(_multi, EL_PORT))
        {
            Serial.println("EL.udp.beginMulticast successful.");
        }
        else
        {
            Serial.println("Reseiver EL.udp.beginMulticast failed."); // localPort
        }

        // profile object
        profile[0x80] = new byte[1 + 1 + 1]{0x80, 1, 0x30};                                                                      // power
        profile[0xd3] = new byte[1 + 1 + 3]{0xd3, 3, 0x00, 0x00, byte(deviceCount)};                                             // total instance number
        profile[0xd4] = new byte[1 + 1 + 2]{0xd4, 2, 0x00, byte(deviceCount + 1)};                                               // total class number
        profile[0xd5] = new byte[1 + 1 + 1 + deviceCount * sizeof(byte[3])]{0xd5, byte(1 + deviceCount * 3), byte(deviceCount)}; // obj list
        profile[0xd6] = new byte[1 + 1 + 1 + deviceCount * sizeof(byte[3])]{0xd6, byte(1 + deviceCount * 3), byte(deviceCount)}; // obj list
        profile[0xd7] = new byte[1 + 1 + 1 + deviceCount * sizeof(byte[2])]{0xd7, byte(1 + deviceCount * 2), byte(deviceCount)}; // class list
        for (int i = 0; i < deviceCount; i++)
        {
            memcpy(&profile[0xd5][3 + i * sizeof(byte[3])], &_eojs[i * sizeof(byte[3])], sizeof(byte[3]));
            memcpy(&profile[0xd6][3 + i * sizeof(byte[3])], &_eojs[i * sizeof(byte[3])], sizeof(byte[3]));
            memcpy(&profile[0xd7][3 + i * sizeof(byte[2])], &_eojs[i * sizeof(byte[3])], sizeof(byte[2]));
        }

        // device object
        details[0x80] = profile[0x80]; // power
    }

    // IP指定による送信
    void send(IPAddress toip, byte sBuffer[], int size)
    {
        debug_printf(true, "send packet(%d) to %s: ", size, toip.toString().c_str());
        debug_dumpln((const char *)sBuffer, size, HEX);
        // Serial.println("EL2::send");
        if (_uniUdp->beginPacket(toip, EL_PORT))
        {
            // Serial.println("UDP beginPacket Successful.");
            _uniUdp->write(sBuffer, size);
        }
        else
        {
            debug_timestamp();
            debug_println("UDP beginPacket failed.");
        }

        if (_uniUdp->endPacket())
        {
            // Serial.println("UDP endPacket Successful.");
        }
        else
        {
            debug_timestamp();
            debug_println("UDP endPacket failed.");
        }
    }
};

class EthernetManager
{
private:
    EL *_echo;
    DataStore *_dataStore;

public:
    EthernetManager(UDP *udp, UDP *uniUdp)
    {
        if (uniUdp != nullptr)
        {
            _echo = new EL2(*udp, *uniUdp, 0x02, 0x88, 0x01);
        }
        else
        {
            _echo = new EL(*udp, 0x02, 0x88, 0x01);
        }
    }
    ~EthernetManager(){};
    void begin()
    {
        _echo->begin();
    }
    inline EL *getEcho() { return _echo; }
    inline void setDataStore(DataStore *ds)
    {
        _dataStore = ds;
        ds->setEthernetManager(this);
    }
    inline DataStore *getDataStore() { return _dataStore; }
    void update()
    {
        int packetSize = 0;
        if (0 != (packetSize = _echo->read()))
        {
            switch (_echo->_rBuffer[EL_ESV])
            {
            case EL_SETC: // 書き込み要求（応答要）
            {
                RequestResponse rr(_echo->remoteIP(), _echo->_rBuffer, packetSize);
                // DataStoreへ委譲
                _dataStore->set(rr);
                _dataStore->get(rr);
            }
            break;
            case EL_GET:     // 読み出し要求
            case EL_INF_REQ: // 値通知要求
            {
                RequestResponse rr(_echo->remoteIP(), _echo->_rBuffer, packetSize);
                // DataStoreへ委譲
                _dataStore->get(rr);
            }
            break;
            case EL_SETI: // 書き込み要求（応答不要）
            {
                RequestResponse rr(_echo->remoteIP(), _echo->_rBuffer, packetSize);
                // DataStoreへ委譲
                _dataStore->set(rr);
            }
            break;
            default:
                break;
            }
        }
    }
};

#endif