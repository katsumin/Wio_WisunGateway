#ifndef _DEVICE_H_
#define _DEVICE_H_
#include <Arduino.h>
#include <map>
#include <time.h>

#define BUF_SIZE 256

class Device
{
private:
    byte _eoj[3];

protected:
    int16_t edtInt16_t(byte *edt) { return (int16_t)(edt[0] << 8 | edt[1]); }
    uint16_t edtUInt16_t(byte *edt) { return (uint16_t)(edt[0] << 8 | edt[1]); }
    int32_t edtInt32_t(byte *edt) { return (int32_t)(edt[0] << 24 | edt[1] << 16 | edt[2] << 8 | edt[3]); }
    uint32_t edtUInt32_t(byte *edt) { return (uint32_t)(edt[0] << 24 | edt[1] << 16 | edt[2] << 8 | edt[3]); }

public:
    Device(byte eoj0, byte eoj1, byte eoj2);
    /**
     * byte[0]: length
     * byte[1]-byte[length]: payload 
     */
    virtual uint8_t *request();
    virtual void parse(const byte *props);
    virtual void parseFrame(const byte *frame);
    inline uint16_t getClassType()
    {
        return _eoj[0] << 8 | _eoj[1];
    }
    inline void setEoj(byte *buf, byte eoj0, byte eoj1, byte eoj2)
    {
        buf[0] = eoj0;
        buf[1] = eoj1;
        buf[2] = eoj2;
    }
};

// 低圧スマート電力量メータ（0x0288）
#define WATT_HOUR_POINTS (48)
#define WATT_HOUR_LAST_POINT (WATT_HOUR_POINTS - 1)
class WattHour
{
private:
    uint32_t _time = 0;
    float _value = 0.0;
    float _values[WATT_HOUR_POINTS]; // 48コマ分

public:
    WattHour();
    static int time2Index(uint32_t epoch);
    inline static int nextIndex(int index) { return (index + 1) % WATT_HOUR_POINTS; };
    inline static int prevIndex(int index) { return (index + WATT_HOUR_LAST_POINT) % WATT_HOUR_POINTS; };
    // void setTime(uint32_t t);
    inline void setTime(uint32_t t) { _time = t; };
    inline uint32_t getTime() { return _time; };
    inline void setValue(float v) { _value = v; };
    inline float getValue() { return _value; };
    // void init();
    void updateValues(float v, uint32_t t);
    inline float getValueAtIndex(int index) { return _values[index]; };
};
class SmartMeter : public Device
{
private:
    uint8_t _cmd_buf[1 + 2 + 2 + 3 + 3 + 1 + 1 + 3 * 2] = {
        sizeof(_cmd_buf) - 1, //
        0x10, 0x81,           // EHD
        0x00, 0x01,           // TID
        0x05, 0xff, 0x01,     // SEOJ
        0x02, 0x88, 0x01,     // DEOJ
        0x62,                 // ESV
        0x03,                 // OPC
        0xe7, 0x00,           // EPC, PDC
        0xe8, 0x00,           // EPC, PDC
        0xe1, 0x00,           // EPC, PDC
        // 0xea, 0x00,           // EPC, PDC
        // 0xeb, 0x00,           // EPC, PDC
    };
    char _buf[256];
    float _k = 0.1;
    int32_t _power = 0;
    float _wattHourPlus = 0;
    uint32_t _timePlus = 0;
    float _wattHourMinus = 0;
    uint32_t _timeMinus = 0;
    char *_statements[4];
    boolean _updated = false;
    WattHour *_plus;
    WattHour *_minus;
    // 瞬時電流（R相）
    short _currentR = 0;
    // 瞬時電流（T相）
    short _currentT = 0;

private:
    void parseEAEB(uint8_t *edt, uint32_t *t, float *p);

public:
    SmartMeter(byte eoj0, byte eoj1, byte eoj2);
    inline int32_t getPower() { return _power; }
    inline void setPower(int32_t value) { _power = value; }
    inline float getWattHourPlus() { return _wattHourPlus; }
    inline float getWattHourMinus() { return _wattHourMinus; }
    inline uint32_t getTimePlus() { return _timePlus; }
    inline uint32_t getTimeMinus() { return _timeMinus; }
    inline WattHour *getWattHourObjPlus() { return _plus; }
    inline WattHour *getWattHourObjMinus() { return _minus; }
    inline void setCurrentR(short value) { _currentR = value; }
    inline short getCurrentR() { return _currentR; };
    inline void setCurrentT(short value) { _currentT = value; }
    inline short getCurrentT() { return _currentT; };
    virtual void parse(const byte *props);
    virtual uint8_t *request() { return _cmd_buf; }
};

#endif