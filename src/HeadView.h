#ifndef _HEADVIEW_H_
#define _HEADVIEW_H_
#include <LGFX_TFT_eSPI.hpp>
#include "Free_Fonts.h"
#include <NTPClient.h>
#include "time.h"

class HeadView
{
private:
    TFT_eSPI *_lcd;
    // IPアドレス
    IPAddress _ipaddr;
    // NW type
    String _nwType;
    // NTPClient
    NTPClient *_ntp;
    // font1高
    int16_t _font1Height;
    char _buf[32];
    void _printNwInfo()
    {
        getLcd()->setTextDatum(TL_DATUM);
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        getLcd()->setTextColor(TFT_YELLOW);
        snprintf(_buf, sizeof(_buf), "%s(%s)", _ipaddr.toString().c_str(), _nwType.c_str());
        int w = getLcd()->textWidth(_buf);
        getLcd()->fillRect(0, 0, w, _font1Height + 2, TFT_BLACK);
        getLcd()->drawString(_buf, 1, 1);
    }
    void _printDate()
    {
        getLcd()->setTextDatum(TR_DATUM);
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        getLcd()->setTextColor(TFT_YELLOW);
        time_t epoch = (time_t)_ntp->getEpochTime();
        tm *t = localtime(&epoch);
        snprintf(_buf, sizeof(_buf), "%04d/%02d/%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        int w = getLcd()->textWidth(_buf);
        getLcd()->fillRect(319 - w, 0, w, _font1Height + 2, TFT_BLACK);
        getLcd()->drawString(_buf, 319, 1);
    }

public:
    HeadView(TFT_eSPI *lcd)
    {
        _lcd = lcd;
    }
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline void setIpAddress(IPAddress addr)
    {
        _ipaddr = addr;
        _printNwInfo();
    };
    inline void setNwType(const char *type)
    {
        _nwType = String(type);
        _printNwInfo();
    };
    inline void setNtp(NTPClient *ntp) { _ntp = ntp; };
    void init()
    {
        getLcd()->setTextFont(1);
        getLcd()->setTextSize(1);
        _font1Height = getLcd()->fontHeight();
    };
    void update()
    {
        _printNwInfo();
        _printDate();
    };
};

#endif