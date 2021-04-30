#include "debug.h"
#include <NTPClient.h>
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>

extern NTPClient ntc;

void debug_init()
{
    Serial.println("sd initialize.");
    if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI))
    {
        Serial.println("initialization failed.");
    }
    Serial.println("Initialization done.");
}

void debug_timestamp()
{
#ifdef DEBUG
    // fs::File f = SD.open("log.txt", FILE_APPEND);
    // if (f)
    // {
    //     f.print(ntc.getFormattedTime());
    //     f.println(" ");
    //     f.close();
    // }
    // else
    // {
    //     Serial.print(ntc.getFormattedTime());
    //     Serial.print(" ");
    // }
    Serial.print(ntc.getFormattedTime());
    Serial.print(" ");
#endif
}

void debug_printfln(boolean with_time, const char *format, ...)
{
#ifdef DEBUG
    char buf[DEBUG_BUF_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (with_time)
        debug_timestamp();
    Serial.println(buf);
#endif
}

void debug_printfln(const char *format, ...)
{
#ifdef DEBUG
    char buf[DEBUG_BUF_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    Serial.println(buf);
#endif
}

void debug_printf(boolean with_time, const char *format, ...)
{
#ifdef DEBUG
    char buf[DEBUG_BUF_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (with_time)
        debug_timestamp();
    Serial.print(buf);
#endif
}

void debug_printf(const char *format, ...)
{
#ifdef DEBUG
    char buf[DEBUG_BUF_SIZE];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    Serial.print(buf);
    va_end(ap);
#endif
}

void debug_println()
{
    Serial.println();
}

void debug_println(unsigned char b, int base)
{
    Serial.println(b, base);
}

void debug_println(int num, int base)
{
    Serial.println(num, base);
}

void debug_println(const String &s)
{
    Serial.println(s);
}

void debug_println(const char c[])
{
    Serial.println(c);
}

void debug_print(unsigned char b, int base)
{
    Serial.print(b, base);
}

void debug_print(int num, int base)
{
    Serial.print(num, base);
}

void debug_print(const String &s)
{
    Serial.print(s);
}

void debug_print(const char c[])
{
    Serial.print(c);
}

void debug_dumpln(const char *buf, size_t s, int base)
{
    for (int i = 0; i < s; i++)
    {
        Serial.print(buf[i], base);
        Serial.print(" ");
    }
    Serial.println(".");
}

void debug_dump(const char *buf, size_t s, int base)
{
    for (int i = 0; i < s; i++)
    {
        Serial.print(buf[i], base);
        Serial.print(" ");
    }
}
