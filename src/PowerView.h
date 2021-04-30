#ifndef _POWERVIEW_H_
#define _POWERVIEW_H_
#include "DataStore.h"
#include "DeviceView.h"
#include "Device.h"

#define WATT_HOUR_FS (3.0f)
#define FS_PIXEL (55)
#define VIEW_TOP_Y (32)
#define AREA_POWER (1 + 106)
#define AREA_CUR_R (AREA_POWER + 106)
#define HEIGHT_FONT_GOTHIC (16)
class PowerView : public DeviceView
{
private:
    // 1文字高
    int16_t _fontHeight;
    // font1高
    int16_t _font1Height;
    int _currentIndex;
    int16_t _fontWidth;

public:
    PowerView(SmartMeter *device, TFT_eSPI *lcd) : DeviceView(device, lcd)
    {
        setName("POWER");
    }
    ~PowerView(){};
    virtual void init()
    {
        // getLcd()->setFreeFont(FF9); // Select the font
        // _fontHeight = getLcd()->fontHeight(GFXFF);
        // _fontHeight = getLcd()->fontHeight(GFXFF);
        getLcd()->setTextFont(&fonts::Font0);
        // getLcd()->setTextSize(1);
        _font1Height = getLcd()->fontHeight();
        // int32_t y = 32;
        getLcd()->fillRect(0, VIEW_TOP_Y, getLcd()->width(), getLcd()->height() - VIEW_TOP_Y - 1, TFT_BLACK);         // ヘッダ以外を消去
        getLcd()->drawRoundRect(0, VIEW_TOP_Y, getLcd()->width(), getLcd()->height() - VIEW_TOP_Y - 1, 5, TFT_WHITE); // 外枠
        getLcd()->drawFastHLine(1, VIEW_TOP_Y + 1 + (HEIGHT_FONT_GOTHIC + 3) * 1, getLcd()->width() - 2, TFT_WHITE);
        getLcd()->drawFastHLine(1, VIEW_TOP_Y + 1 + (HEIGHT_FONT_GOTHIC + 3) * 2, getLcd()->width() - 2, TFT_WHITE); // 区切り線
        _currentIndex = -1;

        // 瞬時値
        // getLcd()->setFreeFont(FF5); // Select the font
        getLcd()->setTextColor(TFT_WHITE);
        // int x = 160 - 18 / 2 * 11;
        getLcd()->setTextFont(&fonts::lgfxJapanGothic_16);
        _fontHeight = getLcd()->fontHeight();
        // Serial.printf("Gothic:%d\tGFXFF:%d", getLcd()->fontHeight(&fonts::lgfxJapanGothic_12), _fontHeight);
        // Serial.println();
        int tx = getLcd()->textWidth("購入電力") / 5;
        getLcd()->setTextDatum(TC_DATUM);
        getLcd()->drawString("購入電力", AREA_POWER / 2, VIEW_TOP_Y + 2);
        getLcd()->drawString("電流Ｒ相", AREA_POWER + (AREA_CUR_R - AREA_POWER) / 2, VIEW_TOP_Y + 2);
        getLcd()->drawString("電流Ｔ相", AREA_CUR_R + (getLcd()->width() - AREA_CUR_R) / 2, VIEW_TOP_Y + 2);
        getLcd()->drawFastVLine(AREA_POWER, VIEW_TOP_Y + 1, (HEIGHT_FONT_GOTHIC + 3) * 2, TFT_WHITE); // 区切り線
        getLcd()->drawFastVLine(AREA_CUR_R, VIEW_TOP_Y + 1, (HEIGHT_FONT_GOTHIC + 3) * 2, TFT_WHITE); // 区切り線
        getLcd()->setTextDatum(BR_DATUM);
        getLcd()->setTextColor(TFT_WHITE);
        getLcd()->drawString("W", AREA_POWER - 3, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
        getLcd()->drawString("A", AREA_CUR_R - 3, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
        getLcd()->drawString("A", getLcd()->width() - 3, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
        _fontWidth = getLcd()->textWidth(" ");
    };
    virtual void update()
    {
        SmartMeter *_sm = (SmartMeter *)getDevice();
        if (isEnable())
        {
            char buf[32];
            // 瞬時値
            getLcd()->setTextFont(&fonts::lgfxJapanGothic_16);
            // getLcd()->setFreeFont(FF5); // Select the font
            // int x = 160 + (-18 / 2 + 12) * 11;
            // getLcd()->drawFastVLine(AREA_POWER, VIEW_TOP_Y + 1, (HEIGHT_FONT_GOTHIC + 3) * 2, TFT_DARKGREY); // 区切り線
            // getLcd()->drawFastVLine(AREA_CUR_R, VIEW_TOP_Y + 1, (HEIGHT_FONT_GOTHIC + 3) * 2, TFT_DARKGREY); // 区切り線
            getLcd()->setTextDatum(BR_DATUM);
            // getLcd()->setTextColor(TFT_WHITE);
            // getLcd()->drawString("W", AREA_POWER - 2, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
            // getLcd()->drawString("A", AREA_CUR_R - 2, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
            // getLcd()->drawString("A", getLcd()->width() - 2, VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2);
            // getLcd()->setTextDatum(TL_DATUM);
            getLcd()->setTextColor(TFT_YELLOW);
            int x = AREA_POWER - 4 - _fontWidth;
            int y = VIEW_TOP_Y + (HEIGHT_FONT_GOTHIC + 3) * 2;
            int w = 7 * _fontWidth;
            snprintf(buf, sizeof(buf), "%6ld", _sm->getPower());
            getLcd()->fillRect(x - w, y - _fontHeight, w, _fontHeight, TFT_BLACK);
            getLcd()->drawString(buf, x, y);
            x = AREA_CUR_R - 4 - _fontWidth;
            snprintf(buf, sizeof(buf), "%6.1f", _sm->getCurrentR() / 10.0f);
            getLcd()->fillRect(x - w, y - _fontHeight, w, _fontHeight, TFT_BLACK);
            getLcd()->drawString(buf, x, y);
            x = getLcd()->width() - 3 - _fontWidth;
            snprintf(buf, sizeof(buf), "%6.1f", _sm->getCurrentT() / 10.0f);
            getLcd()->fillRect(x - w, y - _fontHeight, w, _fontHeight, TFT_BLACK);
            getLcd()->drawString(buf, x, y);
            // 積算値
            getLcd()->setTextDatum(MC_DATUM);
            // getLcd()->setFreeFont(FF9); // Select the font
            getLcd()->setTextFont(&fonts::Font0);
            // getLcd()->setTextSize(1);
            uint32_t epoch = _sm->getWattHourObjPlus()->getTime();
            int index = WattHour::time2Index(epoch);
            // Serial.printf("_currentIndex:%d, index:%d, epoch:%ld", _currentIndex, index, epoch);
            // Serial.println();
            if (_currentIndex != index)
            {
                _currentIndex = index;

                // グラフ表示
                int x = 17;
                int y = 155;
                int w = 5;
                int step_x = w + 1;
                getLcd()->fillRect(x, y - FS_PIXEL, step_x * WATT_HOUR_POINTS + 1, FS_PIXEL * 2 + 2, TFT_BLACK);
                getLcd()->drawFastHLine(x, y, step_x * WATT_HOUR_POINTS, TFT_WHITE);
                getLcd()->drawRect(x - 1, y - FS_PIXEL - 1, step_x * WATT_HOUR_POINTS + 2, FS_PIXEL * 2 + 3, TFT_WHITE);
                getLcd()->drawFastVLine(x - 1 + step_x * 12, y - FS_PIXEL + 1, FS_PIXEL * 2 - 1, TFT_BLUE);
                getLcd()->drawFastVLine(x - 1 + step_x * 24, y - FS_PIXEL + 1, FS_PIXEL * 2 - 1, TFT_BLUE);
                getLcd()->drawFastVLine(x - 1 + step_x * 36, y - FS_PIXEL + 1, FS_PIXEL * 2 - 1, TFT_BLUE);
                for (int i = 0; i < WATT_HOUR_POINTS; i++)
                {
                    int in = (index + i) % WATT_HOUR_POINTS;
                    if (in == WATT_HOUR_LAST_POINT)
                        getLcd()->drawFastVLine(x + i * step_x + w, y - FS_PIXEL + 1, FS_PIXEL * 2 - 1, TFT_YELLOW);
                    float f = _sm->getWattHourObjPlus()->getValueAtIndex(in);
                    int dh = f / WATT_HOUR_FS * FS_PIXEL;
                    dh = (dh > FS_PIXEL - 1) ? FS_PIXEL - 1 : dh;
                    if (f >= 0)
                        getLcd()->fillRect(x + i * step_x, y - dh, w, dh, TFT_GREEN);
                    else
                        getLcd()->drawRect(x + i * step_x, y - FS_PIXEL, w, FS_PIXEL, TFT_DARKGREY);
                    f = _sm->getWattHourObjMinus()->getValueAtIndex(in);
                    dh = f / WATT_HOUR_FS * FS_PIXEL;
                    dh = (dh > FS_PIXEL - 1) ? FS_PIXEL - 1 : dh;
                    if (f >= 0)
                        getLcd()->fillRect(x + i * step_x, y + 1, w, dh, TFT_PINK);
                    else
                        getLcd()->drawRect(x + i * step_x, y + 1, w, FS_PIXEL, TFT_DARKGREY);
                }
                // X軸
                // getLcd()->setTextFont(&fonts::Font0);
                getLcd()->setTextColor(TFT_GREENYELLOW);
                getLcd()->fillRect(1, y + FS_PIXEL + _font1Height * 2 - _font1Height / 2, getLcd()->width() - 2, _font1Height, TFT_BLACK);
                for (int i = 0; i < 5; i++)
                {
                    int step = i * 12;
                    int in = (index + step) % WATT_HOUR_POINTS;
                    snprintf(buf, sizeof(buf), "%02d:%02d", in / 2, (in % 2) * 30);
                    getLcd()->drawString(buf, x - 1 + step_x * step, y + FS_PIXEL + _font1Height * 2 + 1);
                }
                // Y軸
                // getLcd()->setTextFont(&fonts::Font0);
                getLcd()->setTextColor(TFT_WHITE);
                getLcd()->drawString(" 3.0kWh", x - 1, y - FS_PIXEL - _font1Height);
                getLcd()->drawString("0", x - w, y);
                getLcd()->drawString("-3.0kWh", x - 1, y + FS_PIXEL + _font1Height);

                // 最新値表示
                // getLcd()->setTextFont(&fonts::Font0);
                getLcd()->setTextColor(TFT_YELLOW);
                getLcd()->setTextDatum(TL_DATUM);
                int prevIndex = WattHour::prevIndex(index);
                float f = _sm->getWattHourObjPlus()->getValueAtIndex(prevIndex);
                if (f >= 0)
                {
                    snprintf(buf, sizeof(buf), "plus : %7.1fkWh", f);
                    w = getLcd()->textWidth(buf);
                    getLcd()->fillRect(319 - w, y - FS_PIXEL - _font1Height * 2 - 3, w, _font1Height, TFT_BLACK);
                    getLcd()->drawString(buf, 319 - w, y - FS_PIXEL - _font1Height * 2 - 3);
                }
                f = _sm->getWattHourObjMinus()->getValueAtIndex(prevIndex);
                if (f >= 0)
                {
                    snprintf(buf, sizeof(buf), "minus: %7.1fkWh", f * (-1.0f));
                    w = getLcd()->textWidth(buf);
                    getLcd()->fillRect(319 - w, y - FS_PIXEL - _font1Height - 3, w, _font1Height, TFT_BLACK);
                    getLcd()->drawString(buf, 319 - w, y - FS_PIXEL - _font1Height - 3);
                }
            }
        }
    };
};

#endif