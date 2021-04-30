#ifndef _DEBUG_H_
#define _DEBUG_H_
#include <Arduino.h>
#include <stdarg.h>

#define DEBUG_BUF_SIZE 100
void debug_init();
void debug_timestamp();
void debug_printfln(boolean with_time, const char *format, ...);
void debug_printfln(const char *format, ...);
void debug_printf(boolean with_time, const char *format, ...);
void debug_printf(const char *format, ...);
void debug_println();
void debug_println(unsigned char b, int base = 10);
void debug_println(int num, int base = 10);
void debug_println(const String &s);
void debug_println(const char c[]);
void debug_dumpln(const char *buf, size_t s, int base = 10);
void debug_print(unsigned char b, int base = 10);
void debug_print(int num, int base = 10);
void debug_print(const String &s);
void debug_print(const char c[]);
void debug_dump(const char *buf, size_t s, int base = 10);
#endif