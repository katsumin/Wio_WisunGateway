#ifndef _VIEW_H_
#define _VIEW_H_
#include <map>
#include <LGFX_TFT_eSPI.hpp>
#include "FunctionButton.h"

class View
{
private:
    boolean _enable = false;
    TFT_eSPI *_lcd;

public:
    View(TFT_eSPI *lcd)
    {
        _lcd = lcd;
    }
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline boolean isEnable() { return _enable; };
    inline void setEnable(boolean enable) { _enable = enable; };
    virtual void init();
    virtual void update();
    void enter()
    {
        setEnable(true);
        init();
        update();
    };
    void leave()
    {
        setEnable(false);
    };
};

class ViewController
{
private:
    String _curKey;
    String _lastKey;
    std::map<String, View *> _views;
    std::map<String, String> _keys;
    FunctionButton *_button;
    TFT_eSPI *_lcd;

public:
    ViewController(FunctionButton *btn, TFT_eSPI *lcd)
    {
        _curKey = "";
        _lastKey = "";
        _button = btn;
        _lcd = lcd;
    };
    ~ViewController(){};
    inline TFT_eSPI *getLcd() { return _lcd; }
    inline const char *getNextKey() { return _keys[_curKey].c_str(); };
    inline void setCurrentKey(const char *key) { setCurrentKey(String(key)); };
    void setCurrentKey(const String key)
    {
        _curKey = key;
        _button->enable(getNextKey());
    };
    inline const char *getCurrentKey() { return _curKey.c_str(); };
    inline void setView(const char *key, View *view)
    {
        String newKey = String(key);
        _views[newKey] = view;
        if (_lastKey == "")
        {
            _keys[newKey] = newKey;
            _curKey = newKey;
            changeNext();
        }
        else
        {
            String oldNextKey = _keys[_lastKey];
            _keys[_lastKey] = newKey;
            _keys[newKey] = oldNextKey;
            _button->enable(getNextKey());
        }
        _lastKey = newKey;
    };
    void changeNext()
    {
        if (_views.count(_curKey) > 0)
        {
            View *pre = _views[_curKey];
            pre->leave();
        }
        String nextKey = _keys[_curKey];
        if (_views.count(nextKey) > 0)
        {
            View *cur = _views[nextKey];
            cur->enter();
            setCurrentKey(nextKey);
        }
    }
    void update()
    {
        String oldKey = getCurrentKey();
        if (_views.count(oldKey) > 0)
        {
            View *pre = _views[oldKey];
            pre->update();
        }
    }
};

#endif