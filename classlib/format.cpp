#ifndef __INCLUDE_FORMAT
#define __INCLUDE_FORMAT

/**
 * Класс для форматированного вывода текста
 */

#include <avrio.c>

class Format {
protected:

    byte buffer[100];

public:

    // Ссылка на буфер в памяти
    byte* get_buffer() { return buffer; }

    // Конвертирование utf8 -> cp866
    int utf8_to_cp866(const char* s) {

        int  i = 0, cnt = 0;
        byte ch;

        while ( (ch = s[i]) ) {

            // Прописные русские буквы
            if (ch == 0xD0) {

                ch = s[++i];

                if (ch == 0x01) ch = 0xA5;
                else if (ch >= 0x90 && ch < 0xC0) ch -= 0x10;
                else if (ch >= 0xB0 && ch < 0xC0) ch -= 0x10;
            }
            // Строчные русские буквы
            else if (ch == 0xD1) {

                ch = s[++i];

                if (ch == 0x91) ch = 0x85;
                else if (ch >= 0x80) ch += 0x60;
            }

            i++;
            buffer[cnt++] = ch;
        }

        buffer[cnt] = 0;
        return cnt;
    }

    // Печать числа -2147483647 .. 2147483647
    byte i2a(long v) {

        char s[24];
        int  q, i = 0, cnt = 0, cs = 0;

        // Печать символа минус перед числом
        if (v < 0) { v = -v; buffer[cs++] = '-'; cnt = 1; }

        // Вычисление смещения
        do { q = v % 10; v /= 10; s[i++] = '0' + q; } while (v); i--;

        // Вывести число
        for (char k = 0; k <= i; k++) { buffer[cs++] = s[i-k]; cnt++; }

        // В конце ZTerm
        buffer[cs] = 0;

        // Занимаемый размер символов
        return cnt;
    }

/*

    // bits=1 (byte) =2 (word) =4 (dword)
    void hex(unsigned long v, char bytes) {

        int sh = (bytes<<3) - 4;
        for (int i = 0; i < (bytes << 1); i++) {

            unsigned char m = (v >> (sh - 4*i)) & 0x0F;
            printch(m < 10 ? '0' + m : '7' + m);
        }

        return this;
    }

    // Печать float, n=2
    byte f2a(float x, char n) {

        if (x < 0) { x = -x; printch('-'); }

        unsigned long i = (unsigned long) x;
        float f = x - i;

        // Печать целочисленного значения
        printint(i);
        printch('.');

        // Печать остатка
        for (int k = 0; k < n; k++) {

            f *= 10;
            printch(f + '0');
            f = f - (int)f;
        }

        return this;
    }

    // Печать 2-х знаков после запятой
    byte f2a(float x) { return f2a(x, 2); }
*/
};

#endif
