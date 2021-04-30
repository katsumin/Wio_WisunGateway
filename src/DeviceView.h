#ifndef _DEVICEVIEW_H_
#define _DEVICEVIEW_H_
#include "View.h"
#include "Device.h"

class DeviceView : public View
{
private:
    Device *_device;
    char *_name;

protected:
    int16_t _baseY;
    // 1文字高
    int16_t _fontHeight;
    // 1文字幅
    int16_t _fontWidth;

public:
    DeviceView(Device *device, TFT_eSPI *lcd);
    inline void setDevice(Device *device) { _device = device; }
    inline void setName(char *name) { _name = name; }
    inline char *getName() { return _name; }
    virtual inline Device *getDevice() { return _device; }
    virtual void init();
    virtual void update() {}
};

#endif