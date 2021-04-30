#ifndef _DATASTORE_H_
#define _DATASTORE_H_
#include <map>
#include "EL.h"
#include "Device.h"
#include "RequestResponse.h"
#include <set>

class EthernetManager;
class WisunManager;

#define BUF_SIZE 1024
class DataStore
{
private:
    std::set<byte> _nocache = {
        0x81, // 設置場所
        0x88, // 異常発生状態
        0x97, // 現在時刻
        0x98, // 現在年月日
        0xe0, // 積算電力量計測値（正方向）
        0xe2, // 積算電力量計測値履歴１（正方向）
        0xe3, // 積算電力量計測値（逆方向）
        0xe4, // 積算電力量計測値履歴１（逆方向）
        0xe5, // 積算履歴収集日１
        0xec, // 積算電力量計測値履歴２
        0xed, // 積算履歴収集日２
    };
    std::map<String, RequestResponse *> _delegate;
    Device *_device;
    EL *_echo;
    WisunManager *_wm = nullptr;
    EthernetManager *_em = nullptr;

public:
    DataStore(Device *device)
    {
        _device = device;
    }
    void get(RequestResponse &rr);
    void set(RequestResponse &rr);
    inline void setEcho(EL *echo) { _echo = echo; }
    inline EL *getEcho() { return _echo; }
    inline void setDevice(Device *device) { _device = device; }
    inline Device *getDevice() { return _device; }
    inline void setEthernetManager(EthernetManager *em) { _em = em; }
    inline void setWisunManager(WisunManager *wm) { _wm = wm; }
};

#endif