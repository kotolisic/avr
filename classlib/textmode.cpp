#include <avrio.c>
#include "format.cpp"

class TextMode {

protected:

    byte cursor_x, cursor_y, cursor_cl, cursor_show;
    Format form;

public:

    // Инициализация
    TextMode() { }

    // Запуск текстового режима
    TextMode* start() {

        cursor_show = 1;

        outp(BANK_LO,   0);
        outp(BANK_HI,   0);
        outp(VIDEOMODE, VM_80x25);

        cursor(0, 0);
        return this;
    }

    // Установка текстового курсора в нужную позицию
    TextMode* cursor(byte x, byte y) {

        cursor_x = x;
        cursor_y = y;

        if (cursor_show) {
            outp(CURSOR_X, x);
            outp(CURSOR_Y, y);
        }

        return this;
    }

    // Скрыть курсор
    TextMode* hide() {

        cursor_show = 0;
        outp(CURSOR_Y, 25);
        return this;
    }

    // Показать курсор
    TextMode* show() {

        cursor_show = 1;
        cursor(cursor_x, cursor_y);
        return this;
    }

    // Очистка экрана
    TextMode* cls(byte cl) {

        heap(vm, 0xF000);

        cursor(0, 0);
        cursor_cl = cl;

        for (int i = 0; i < 2000; i++) {
            vm[2*i+0] = 0;
            vm[2*i+1] = cl;
        }
        return this;
    }

    // Текущий цвет
    TextMode* color(unsigned char cl) {
        cursor_cl = cl;
        return this;
    }

    // Печать символа на экране
    TextMode* printc(byte x, byte y, char ch) {

        heap(vm, 0xF000);
        int   z = (x<<1) + (y<<7) + (y<<5);

        vm[z]   = ch;
        vm[z+1] = cursor_cl;
        return this;
    }

    // Печать в режиме телетайпа
    TextMode* printch(byte s) {

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
        return this;
    }

    // Печать строки
    int print(const char* s) {

        int i = 0;
        while (s[i]) {
            printch(s[i++]);
        }
        return i;
    }

    // Печать UTF8-строки русской
    int printutf8(const char* s) {

        form.utf8_to_cp866(s);
        return print((const char*) form.get_buffer());
    }

    // Печать с переносом строки на новую
    TextMode* println(const char *s) {

        print(s);
        printch(10);
        return this;
    }

    // Обновить палитру
    TextMode* palette(byte id, byte r, byte g, byte b) {

        heap(vm, 0xFFA0);

        vm[2*id + 0] = (b >> 4) | (g & 0xF0);
        vm[2*id + 1] = (r >> 4);

        return this;
    }

    // Рисовать фрейм
    TextMode* frame(byte x1, byte y1, byte x2, byte y2, byte density) {

        int i;
        char v = density ? 0xBA : 0xB3;
        char h = density ? 0xCD : 0xC4;

        for (i = x1 + 1; i < x2; i++) { printc(i, y1, h); printc(i, y2, h); }
        for (i = y1 + 1; i < y2; i++) { printc(x1, i, v); printc(x2, i, v); }

        if (density) {

            printc(x1, y1, 0xC9); printc(x2, y1, 0xBB);
            printc(x1, y2, 0xC8); printc(x2, y2, 0xBC);

        } else {

            printc(x1, y1, 0xDA); printc(x2, y1, 0xBF);
            printc(x1, y2, 0xC0); printc(x2, y2, 0xD9);
        }

        return this;
    }

    // Рисование блока текста
    TextMode* block(byte x1, byte y1, byte x2, byte y2, byte cl) {

        cursor_cl = cl;
        for (int i = y1; i <= y2; i++)
        for (int j = x1; j <= x2; j++)
            printc(j, i, ' ');

        return this;
    }

    TextMode* printint(long v) {

        form.i2a(v);
        print((const char*) form.get_buffer());
        return this;
    }
    
    // printfloat 1
    // printfloat 2
    // printhex
};
