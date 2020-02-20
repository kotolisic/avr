#include <avrio.c>

class TextMode {

protected:

    byte cursor_x, cursor_y, cursor_cl;

public:

    // Инициализация
    TextMode() { }

    // Запуск текстового режима
    void start() {

        outp(VIDEOMODE, VM_80x25);
        cursor(0, 0);
    }

    // Установка текстового курсора в нужную позицию
    void cursor(byte x, byte y) {

        cursor_x = x;
        cursor_y = y;

        outp(CURSOR_X, x);
        outp(CURSOR_Y, y);
    }

    // Очистка экрана
    void cls(byte cl) {

        heap(vm, 0xF000);

        cursor(0, 0);
        cursor_cl = cl;

        for (int i = 0; i < 2000; i++) {
            vm[2*i+0] = 0;
            vm[2*i+1] = cl;
        }
    }

    // Текущий цвет
    void color(unsigned char cl) {
        cursor_cl = cl;
    }

    // Печать символа не в телетайпе
    void printc(byte x, byte y, char ch) {

        heap(vm, 0xF000);
        int   z = (x<<1) + (y<<7) + (y<<5);

        vm[z]   = ch;
        vm[z+1] = cursor_cl;
    }

    // Печать в режиме телетайпа
    void printch(byte s) {

        int i;
        heap(vm, 0xF000);

        if (s == 10) {
            cursor_x = 80;
        } else {
            printc(cursor_x, cursor_y, s);
            cursor_x++;
        }

        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }

        // Скроллинг вверх
        if (cursor_y >= 25) {

            for (i = 0; i < 4000 - 160; i += 2) {
                vm[i]   = vm[i + 160];
                vm[i+1] = vm[i + 161];
            }

            // Очистка новой строки
            for (i = 4000 - 160; i < 4000; i += 2) {
                vm[i]   = ' ';
                vm[i+1] = cursor_cl;
            }

            cursor_y = 24;
        }

        cursor(cursor_x, cursor_y);
    }

    // Печать строки
    void print(const char* s) {

        int i = 0;
        while (s[i]) {
            printch(s[i++]);
        }
    }

    // Печать с переносом строки на новую
    void println(const char *s) {

        print(s);
        printch(10);
    }

    // -----------------------------------------------------------------

    // Печать числа -2147483647 .. 2147483647
    char printint(long v) {

        char s[16];
        int  q, i = 0, cnt = 0;

        // Печать символа минус перед числом
        if (v < 0) { v = -v; printch('-'); cnt = 1; }

        // Вычисление смещения
        do { q = v % 10; v /= 10; s[i++] = '0' + q; } while (v); i--;

        // Вывести число
        for (char k = 0; k <= i; k++) { printch(s[i-k]); cnt++; }

        // Занимаемый размер символов
        return cnt;
    }

    // bits=1 (byte) =2 (word) =4 (dword)
    void printhex(unsigned long v, char bytes) {

        int sh = (bytes<<3) - 4;
        for (int i = 0; i < (bytes << 1); i++) {

            unsigned char m = (v >> (sh - 4*i)) & 0x0F;
            printch(m < 10 ? '0' + m : '7' + m);
        }
    }

    // Печать float, n=2
    void printfloat(float x, char n) {

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
    }

    // Печать 2-х знаков после запятой
    void printfloat(float x) { printfloat(x, 2); }
};
