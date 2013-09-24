#include <Arduino.h>

#ifdef DEBUG
    int debug_putc(char c, FILE *) { Serial.write(c); return c; }
    void debug_begin(void) { fdevopen(&debug_putc, 0); }
    #define D(f, ...) do { printf_P(PSTR(f "\n"), ##__VA_ARGS__); } while (0)
#else
    #define debug_begin()
    #define D(...)
#endif
