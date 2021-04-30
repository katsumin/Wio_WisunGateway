#ifndef _REQUEST_RESPONSE_H_
#define _REQUEST_RESPONSE_H_
#include <Arduino.h>
#include <IPAddress.h>
#include "debug.h"
#include <set>
#include "EL.h"

class RequestResponse
{
private:
    std::set<IPAddress> _addrs;
    uint8_t *_frame = nullptr;
    int _size;
    uint32_t _timestamp = 0;

public:
    inline uint8_t *getFrame() { return _frame; }
    inline std::set<IPAddress> getAddresses() { return _addrs; }
    inline int getSize() { return _size; }
    inline void appendAddress(IPAddress addr) { _addrs.insert(addr); }
    inline uint32_t getTimestamp() { return _timestamp; }
    inline void setTimestamp(uint32_t seq) { _timestamp = seq; }
    String getKey(int index)
    {
        String key = String(_frame[index + 0], HEX);
        key += String(_frame[index + 1], HEX);
        key += '_';
        int opc = _frame[EL_OPC];
        uint8_t *data = &_frame[EL_EPC];
        for (int i = 0; i < opc; i++)
        {
            key += String(data[0], HEX);
            uint8_t pdc = data[1];
            data += pdc + 2;
        }
        return key;
    }
    RequestResponse() {}
    RequestResponse(IPAddress addr, uint8_t buf[], int size)
    {
        _addrs.insert(addr);
        _size = size;
        _frame = new uint8_t[_size];
        _timestamp = millis();
        memcpy(_frame, buf, size);
        debug_printfln(true, "constructor RequestResponse: %p, %dbyte, timestamp=%ld, from %s", _frame, size, _timestamp, addr.toString().c_str());
    }
    RequestResponse(const RequestResponse &src)
    {
        _addrs = src._addrs;
        _size = src._size;
        _frame = new uint8_t[_size];
        _timestamp = src._timestamp;
        memcpy(_frame, src._frame, _size);
        debug_printf(true, "copy RequestResponse _frame:%p, src._frame:%p\t", _frame, src._frame);
        debug_dumpln((const char *)_frame, _size, HEX);
    }
    ~RequestResponse()
    {
        if (_frame != nullptr)
        {
            debug_printfln(true, "destructor RequestResponse: %p", _frame);
            delete[] _frame;
            _addrs.clear();
        }
    }
};

#endif