#include <avrio.c>
#include "format.cpp"

class Graphics {

protected:

    int  region_x1, region_y1, region_x2, region_y2;
    int  cursor_x, cursor_y;
    byte cursor_cl;
    Format form;

    // Вычисление адреса и банка
    word bank8(int x, int y) {

        unsigned int z = (y << 7) + (y << 5) + (x >> 1);
        outp(BANK_LO, 8 | ((z >> 12) & 7));
        return z & 0xFFF;
    }

public:

    // Запустить видеорежим
    Graphics* start() {

        outp(BANK_LO,   0);
        outp(BANK_HI,   0);
        outp(VIDEOMODE, VM_320x200x4);

        region_x1 = 0;
        region_y1 = 0;
        region_x2 = 319;
        region_y2 = 199;
        cursor_x  = 0;
        cursor_y  = 0;
        cursor_cl = 15;

        return this;
    }

    // Установить точку
    Graphics* pset(int x, int y, byte cl) {

        heap(vm, 0xf000);

        if (x < region_x1 || y < region_y1 || x > region_x2 || y > region_y2)
            return this;

        word z = bank8(x, y);

        cl   &= 15;
        vm[z] = x & 1 ? ((vm[z] & 0xF0) | cl) : ((vm[z] & 0x0F) | (cl << 4));

        return this;
    }

    // Вернуть точку
    byte point(int x, int y) {

        heap(vm, 0xf000);

        word z  = bank8(x, y);
        return x & 1 ? vm[z] & 0x0F : (vm[z] >> 4);
    }

    // Рисование блока. Необходимо, чтобы x1 < x2, y1 < y2
    Graphics* block(int x1, int y1, int x2, int y2, byte cl) {

        heap(vm, 0xf000);

        // Расчет инициирующей точки
        word  xc = (x2>>1) - (x1>>1);     // Расстояние
        word  cc = cl | (cl << 4);        // Сдвоенный цвет
        word  z  = bank8(x1, y1);
        word  zc;

        // Коррекции, если не попадает
        if (x1 & 1) { z++; xc--; }
        if (x2 & 1) { xc++; }

        // Первичный банк памяти
        byte bank = inp(BANK_LO), zb;

        // Построение линии сверху вниз
        for (int i = y1; i <= y2; i++) {

            // Выставить текущий банк
            outp(BANK_LO, bank);

            // Сохранение предыдущих указателей
            zc = z;
            zb = bank;

            // Рисование горизонтальной линии
            for (word j = 0; j < xc; j++) {

                vm[zc++] = cc;
                if (zc == 0x1000) { outp(BANK_LO, ++bank); zc = 0; }
            }
            bank = zb;

            // К следующему Y++
            z += 160;

            // Перелистнуть страницу, если нужно
            if (z >= 0x1000) { z &= 0xFFF; bank++; }
        }

        // Дорисовать линии слева и справа
        if ( (x1 & 1)) for (int i = y1; i <= y2; i++) pset(x1, i, cl);
        if (!(x2 & 1)) for (int i = y1; i <= y2; i++) pset(x2, i, cl);

        return this;
    }

    // Рисование линии
    Graphics* line(int x1, int y1, int x2, int y2, byte cl) {

        if (y2 < y1) {
            x1 ^= x2; x2 ^= x1; x1 ^= x2;
            y1 ^= y2; y2 ^= y1; y1 ^= y2;
        }

        int deltax = x2 > x1 ? x2 - x1 : x1 - x2;
        int deltay = y2 - y1;
        int signx  = x1 < x2 ? 1 : -1;

        int error2;
        int error = deltax - deltay;

        while (x1 != x2 || y1 != y2)
        {
            pset(x1, y1, cl);
            error2 = error * 2;

            if (error2 > -deltay) {
                error -= deltay;
                x1 += signx;
            }

            if (error2 < deltax) {
                error += deltax;
                y1 += 1;
            }
        }

        pset(x1, y1, cl);
        return this;
    }

    // Рисование окружности
    Graphics* circle(int xc, int yc, int r, byte c) {

        int x = 0;
        int y = r;
        int d = 3 - 2*y;

        while (x <= y) {

            // --
            pset(xc - x, yc + y, c);
            pset(xc + x, yc + y, c);
            pset(xc - x, yc - y, c);
            pset(xc + x, yc - y, c);
            pset(xc + y, yc + x, c);
            pset(xc - y, yc + x, c);
            pset(xc + y, yc - x, c);
            pset(xc - y, yc - x, c);
            // ...

            d += 4*x + 6;
            if (d >= 0) {
                d += 4*(1 - y);
                y--;
            }

            x++;
        }

        return this;
    }

    // Печать символа
    Graphics* printchar(int x, int y, byte ch, byte cl) {

        byte t[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

        heap(vm, 0xf000);
        for (int i = 0; i < 16; i++) {

            bank(1);
            byte fn = vm[16*ch + i];
            for (int j = 0; j < 8; j++) {
                if (fn & t[j & 7])
                    pset(x + j, y + i, cl);
            }
        }

        return this;
    }

    // Установка курсора
    Graphics* cursor(int x, int y) {

        cursor_x = x;
        cursor_y = y;

        return this;
    }

    // Выбор цвета
    Graphics* color(byte cl) {
        cursor_cl = cl;
        return this;
    }

    // Печать в режиме телетайпа
    Graphics* printch(byte cl) {

        printchar(cursor_x, cursor_y, cl, cursor_cl);

        cursor_x += 8;
        if (cursor_x > region_x2) {
            cursor_x = region_x1;
            cursor_y += 16;
            // @todo region move
        }

        return this;
    }

    // Печать строки (utf8)
    int print(const char* s) {

        form.utf8_to_cp866(s);
        byte* b = form.get_buffer();

        int i = 0; while (b[i]) printch(b[i++]); return i;
    }

    // Формы и окна
    // -----------------------------------------------------------------

    // Рисование окна
    Graphics* window(int x, int y, int w, int h, const char* s) {

        int x2 = x + w, y2 = y + h;

        block(x, y, x2, y2, 7);
        line(x2, y, x2, y2, 8);
        line(x, y2, x2, y2, 8);
        line(x, y, x2, y, 15);
        line(x, y, x, y2, 15);
        block(x+2,y+2,x2-2,y+18,3);

        cursor(x+4,y+2)->color(15);
        print(s);

        return this;
    }
};
