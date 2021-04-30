#include "FunctionButton.h"

Button BtnA(WIO_KEY_C, true, DEBOUNCE_MS);
Button BtnB(WIO_KEY_B, true, DEBOUNCE_MS);
Button BtnC(WIO_KEY_A, true, DEBOUNCE_MS);

FunctionButton::FunctionButton(Button *button, TFT_eSPI *lcd, uint16_t xpos)
{
    _button = button;
    _lcd = lcd;
    _xpos = xpos;
}

void FunctionButton::_set(const char *label, int color)
{
    _label = (char *)label;
    int32_t x_pos = _xpos;
    // int32_t y_pos = getLcd()->height() - TEXT_HEIGHT;
    int32_t y_pos = TEXT_HEIGHT;
    getLcd()->setTextDatum(TL_DATUM);
    getLcd()->setTextFont(1);
    getLcd()->setTextSize(1);
    getLcd()->fillRect(x_pos, y_pos - 1, WIDTH, TEXT_HEIGHT, TFT_BLACK);
    getLcd()->setTextColor(color);
    int32_t x = x_pos + WIDTH / 2 - getLcd()->textWidth(label) / 2;
    int32_t y = y_pos + 3;
    getLcd()->drawString(label, x, y);
    getLcd()->drawRoundRect(x_pos, y_pos - 1, WIDTH, TEXT_HEIGHT, 2, TFT_WHITE);
}

void FunctionButton::update()
{
    BtnA.read();
    BtnB.read();
    BtnC.read();
}