#include "DeviceView.h"

//
DeviceView::DeviceView(Device *device, TFT_eSPI *lcd) : View(lcd)
{
    setDevice(device);
}

//
void DeviceView::init()
{
    getLcd()->setFont(&fonts::lgfxJapanGothic_16);
    int16_t fh = getLcd()->fontHeight();
    int32_t y = 16;
    getLcd()->fillRect(0, y, getLcd()->width(), getLcd()->height() - y * 2, TFT_BLACK);             // ヘッダ以外を消去
    getLcd()->drawRoundRect(0, y, getLcd()->width(), getLcd()->height() - y * 2 - 4, 5, TFT_WHITE); // 外枠
    _baseY = y + fh + 3;
    getLcd()->drawFastHLine(0, _baseY - 1, getLcd()->width());
}
