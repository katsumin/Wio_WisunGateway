#include <Arduino.h>
#include "Device.h"
#include "EL.h"
#include "debug.h"

// base
Device::Device(byte eoj0, byte eoj1, byte eoj2)
{
    setEoj(_eoj, eoj0, eoj1, eoj2);
}

// SmartMeter
WattHour::WattHour()
{
    for (int i = 0; i < WATT_HOUR_POINTS; i++)
        _values[i] = -1;
}
int WattHour::time2Index(uint32_t epoch)
{
    long t = epoch % (60 * 60 * 24);
    t /= (30 * 60);
    return (int)t;
};
void WattHour::updateValues(float v, uint32_t t)
{
    float preValue = getValue();
    uint32_t preTime = getTime();
    if (preTime == t)
        return;
    if (preValue > 0)
    {
        // 測定間隔が空いたコマを無効値で埋める
        int index = time2Index(preTime);
        int diff = time2Index(t - preTime) - 1;
        diff = (diff > WATT_HOUR_LAST_POINT) ? WATT_HOUR_LAST_POINT : diff;
        for (int i = 0; i < diff; i++)
        {
            index %= WATT_HOUR_POINTS;
            _values[index++] = -1;
        }
    }
    // 最新値で更新
    int curIndex = prevIndex(time2Index(t));
    setTime(t);
    _values[curIndex] = v - preValue;
    setValue(v);
};
SmartMeter::SmartMeter(byte eoj0, byte eoj1, byte eoj2) : Device(eoj0, eoj1, eoj2)
{
    // setEoj(&_cmd_buf[8], eoj0, eoj1, eoj2);
    _plus = new WattHour();
    _minus = new WattHour();
}

void SmartMeter::parseEAEB(uint8_t *edt, uint32_t *t, float *p)
{
    uint32_t v = edtUInt32_t(&edt[7]);
    if (v <= 99999999L)
    {
        tm tm;
        tm.tm_year = (edt[0] << 8 | edt[1]) - 1900;
        tm.tm_mon = edt[2] - 1;
        tm.tm_mday = edt[3];
        tm.tm_hour = edt[4];
        tm.tm_min = edt[5];
        tm.tm_sec = edt[6];
        *t = mktime(&tm);
        *p = (float)v * _k;
        debug_printfln(true, "%04d/%02d/%02d %02d:%02d:%02d, %10.2f[kWh]", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, *p);
        _updated = true;
    }
}

void SmartMeter::parse(const byte *props)
{
    byte epc = props[EL_EPC - EL_EPC];
    byte pdc = props[EL_PDC - EL_EPC];
    byte *edt = (byte *)&props[EL_EDT - EL_EPC];
    switch (epc)
    {
    case 0xe1:
        switch (edt[0])
        {
        case 0x00:
            _k = 1.0;
            break;
        case 0x01:
            _k = 0.1;
            break;
        case 0x02:
            _k = 0.01;
            break;
        case 0x03:
            _k = 0.001;
            break;
        case 0x04:
            _k = 0.0001;
            break;
        case 0x0a:
            _k = 10.0;
            break;
        case 0x0b:
            _k = 100.0;
            break;
        case 0x0c:
            _k = 1000.0;
            break;
        case 0x0d:
            _k = 10000.0;
            break;
        default:
            break;
        }
    case 0xe7:
        if (pdc == 4)
        {
            int32_t d = edtInt32_t(edt);
            if (-2147483647 <= d && d <= 2147483645)
            {
                _power = d;
                debug_printfln(true, "消費電力: %10ld[w]", _power);
            }
        }
        break;
    case 0xe8:
        if (pdc == 4)
        {
            int16_t r = edtInt16_t(&edt[0]);
            int16_t t = edtInt16_t(&edt[2]);
            if (-32767 <= r && r <= 32765)
            {
                setCurrentR(r);
                setCurrentT(t);
                debug_printfln(true, "電流R相: %7.1f[A], 電流T相: %7.1f[A]", _currentR / 10.0f, _currentT / 10.0f);
            }
        }
        break;
    case 0xea:
        if (pdc == 11)
        {
            parseEAEB(edt, &_timePlus, &_wattHourPlus);
            _plus->updateValues(_wattHourPlus, _timePlus);
        }
        break;
    case 0xeb:
        if (pdc == 11)
        {
            parseEAEB(edt, &_timeMinus, &_wattHourMinus);
            _minus->updateValues(_wattHourMinus, _timeMinus);
        }
        break;
    default:
        break;
    }
}

void Device::parseFrame(const byte *frame)
{
    byte opc = frame[EL_OPC];
    byte *data = (byte *)&frame[EL_EPC];
    for (int i = 0; i < opc; i++)
    {
        byte pdc = data[1];
        parse(data);
        data += pdc + 2;
    }
}
