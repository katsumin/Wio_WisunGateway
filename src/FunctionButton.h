#ifndef _FUNC_BTN_H_
#define _FUNC_BTN_H_
#include <LGFX_TFT_eSPI.hpp>
#include "utility/Button.h"

#define TEXT_HEIGHT (15)
#define WIDTH (80)
#define POS_A_X (0)
#define POS_B_X (POS_A_X + WIDTH + 10)
#define POS_C_X (POS_B_X + WIDTH + 10)
#define DEBOUNCE_MS (5)

class FunctionButton
{
private:
    TFT_eSPI *_lcd;
    Button *_button;
    char *_label;
    void _set(const char *label, int color);
    boolean _enable = false;
    uint16_t _xpos;

public:
    FunctionButton(Button *button, TFT_eSPI *lcd, uint16_t xpos);
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline char *getLabel() { return _label; };
    inline Button *getButton()
    {
        return _button;
    };
    inline void enable(const char *label)
    {
        _set(label, TFT_WHITE);
        _enable = true;
    };
    inline void disable(const char *label)
    {
        _set(label, TFT_MAGENTA);
        _enable = false;
    };
    inline boolean isEnable() { return _enable; };
    static void update();
};

extern Button BtnA;
extern Button BtnB;
extern Button BtnC;
#endif
