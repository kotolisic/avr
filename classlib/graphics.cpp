#include <avrio.c>
#include "format.cpp"

// Таблица m=i*160
static const word bank_lut[] = {

    0x0000, 0x00a0, 0x0140, 0x01e0, 0x0280, 0x0320, 0x03c0, 0x0460,
    0x0500, 0x05a0, 0x0640, 0x06e0, 0x0780, 0x0820, 0x08c0, 0x0960,
    0x0a00, 0x0aa0, 0x0b40, 0x0be0, 0x0c80, 0x0d20, 0x0dc0, 0x0e60,
    0x0f00, 0x0fa0, 0x1040, 0x10e0, 0x1180, 0x1220, 0x12c0, 0x1360,
    0x1400, 0x14a0, 0x1540, 0x15e0, 0x1680, 0x1720, 0x17c0, 0x1860,
    0x1900, 0x19a0, 0x1a40, 0x1ae0, 0x1b80, 0x1c20, 0x1cc0, 0x1d60,
    0x1e00, 0x1ea0, 0x1f40, 0x1fe0, 0x2080, 0x2120, 0x21c0, 0x2260,
    0x2300, 0x23a0, 0x2440, 0x24e0, 0x2580, 0x2620, 0x26c0, 0x2760,
    0x2800, 0x28a0, 0x2940, 0x29e0, 0x2a80, 0x2b20, 0x2bc0, 0x2c60,
    0x2d00, 0x2da0, 0x2e40, 0x2ee0, 0x2f80, 0x3020, 0x30c0, 0x3160,
    0x3200, 0x32a0, 0x3340, 0x33e0, 0x3480, 0x3520, 0x35c0, 0x3660,
    0x3700, 0x37a0, 0x3840, 0x38e0, 0x3980, 0x3a20, 0x3ac0, 0x3b60,
    0x3c00, 0x3ca0, 0x3d40, 0x3de0, 0x3e80, 0x3f20, 0x3fc0, 0x4060,
    0x4100, 0x41a0, 0x4240, 0x42e0, 0x4380, 0x4420, 0x44c0, 0x4560,
    0x4600, 0x46a0, 0x4740, 0x47e0, 0x4880, 0x4920, 0x49c0, 0x4a60,
    0x4b00, 0x4ba0, 0x4c40, 0x4ce0, 0x4d80, 0x4e20, 0x4ec0, 0x4f60,
    0x5000, 0x50a0, 0x5140, 0x51e0, 0x5280, 0x5320, 0x53c0, 0x5460,
    0x5500, 0x55a0, 0x5640, 0x56e0, 0x5780, 0x5820, 0x58c0, 0x5960,
    0x5a00, 0x5aa0, 0x5b40, 0x5be0, 0x5c80, 0x5d20, 0x5dc0, 0x5e60,
    0x5f00, 0x5fa0, 0x6040, 0x60e0, 0x6180, 0x6220, 0x62c0, 0x6360,
    0x6400, 0x64a0, 0x6540, 0x65e0, 0x6680, 0x6720, 0x67c0, 0x6860,
    0x6900, 0x69a0, 0x6a40, 0x6ae0, 0x6b80, 0x6c20, 0x6cc0, 0x6d60,
    0x6e00, 0x6ea0, 0x6f40, 0x6fe0, 0x7080, 0x7120, 0x71c0, 0x7260,
    0x7300, 0x73a0, 0x7440, 0x74e0, 0x7580, 0x7620, 0x76c0, 0x7760,
    0x7800, 0x78a0, 0x7940, 0x79e0, 0x7a80, 0x7b20, 0x7bc0, 0x7c60,
};

class Graphics {

protected:

    int  region_x1, region_y1, region_x2, region_y2;
    int  cursor_x, cursor_y;
    byte cursor_cl, wipe_under;
    Format form;

    // Вычисление адреса и банка
    word bank8(int x, int y) {

        //word z = (y<<7) + (y<<5) + (x>>1);
        word z = bank_lut[y] + (x>>1);
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
        wipe_under = 0;

        return this;
    }

    // Очистка экрана
    Graphics* cls(byte cl) {

        block(0, 0, 319, 199, cl);
        return this;
    }    

    // Установить точку
    Graphics* pset(int x, int y, byte cl) {

        heap(vm, 0xf000);
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

    // Блок линии
    Graphics* lineb(int x1, int y1, int x2, int y2, byte cl) {

        line(x1, y1, x2, y1, cl);
        line(x2, y1, x2, y2, cl);
        line(x2, y2, x1, y2, cl);
        line(x1, y2, x1, y1, cl);
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
                else if (wipe_under & 0x10) {
                    pset(x + j, y + i, wipe_under & 15);
                }
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

    // Очищать ли background и в какой цвет (младший ниббл)
    Graphics* wiped(byte w) {

        wipe_under = w;
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

    void printint(long v) {

        form.i2a(v);
        print((const char*) form.get_buffer());
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
