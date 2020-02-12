#include "fonts/ansi3.h"

// Печать символа 4x8 на экране. Допустиы только 0x20 - 0x7E
void print4c(char ch) {

    if (ch < 0x20 || ch >= 0x80)
        return;

    ch -= 0x20;
    for (int i = 0; i < 8; i++) {

        char cb = ansi3[ch >> 1][i];
             cb = ch & 1 ? cb & 15 : cb >> 4;

        for (int j = 0; j < 4; j++) {
            if (cb & (1 << (3 - j)))
                pset(g_cursor_x + j, g_cursor_y + i, g_cursor_cl);
        }
    }

    g_cursor_x += 4;

    // Перемотка
    if (g_cursor_x >= 320) {

        g_cursor_x = 0;
        g_cursor_y++;

        // Нужна ли тут перемотка экрана?
        if (g_cursor_y >= 200) {
            g_cursor_y = 192;
        }
    }
}

// Печать строки на экране
void print4(char* s) {

    while (*s) { print4c(*s); s++; }
}

// bits=1 (byte) =2 (word) =4 (dword)
void print4hex(unsigned long v, char bytes) {

    int sh = (bytes<<3) - 4;
    for (int i = 0; i < (bytes << 1); i++) {

        unsigned char m = (v >> (sh - 4*i)) & 0x0F;
        print4c(m < 10 ? '0' + m : '7' + m);
    }
}

// Печать числа -2147483647 .. 2147483647
char print4l(long v) {

    char s[16];
    int  q, i = 0, cnt = 0;

    // Печать символа минус перед числом
    if (v < 0) { v = -v; print4c('-'); cnt = 1; }

    // Вычисление смещения
    do { q = v % 10; v /= 10; s[i++] = '0' + q; } while (v); i--;

    // Вывести число
    for (char k = 0; k <= i; k++) { print4c(s[i-k]); cnt++; }

    // Занимаемый размер символов
    return cnt;
}

