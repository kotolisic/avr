#include <avrio.c>

class Graphics {

protected:

    // Вычисление адреса и банка
    word _bank8(int x, int y) {

        unsigned int z = (y << 7) + (y << 5) + (x >> 1);
        outp(BANK_LO, 8 | ((z >> 12) & 7));
        return z & 0xFFF;
    }

    int region_x1, region_y1, region_x2, region_y2;

public:

    void start() {

        outp(BANK_LO,   0);
        outp(BANK_HI,   0);
        outp(VIDEOMODE, VM_320x200x4);

        region_x1 = 0;
        region_y1 = 0;
        region_x2 = 319;
        region_y2 = 199;
    }

    void pset(int x, int y, byte cl) {

        heap(vm, 0xf000);

        if (x < region_x1 || y < region_y1 || x > region_x2 || y > region_y2)
            return;

        word z = _bank8(x, y);

        cl   &= 15;
        vm[z] = x & 1 ? ((vm[z] & 0xF0) | cl) : ((vm[z] & 0x0F) | (cl << 4));
    }

    // Вернуть точку
    byte point(int x, int y) {

        heap(vm, 0xf000);

        word z  = _bank8(x, y);
        return x & 1 ? vm[z] & 0x0F : (vm[z] >> 4);
    }

    // Рисование блока. Необходимо, чтобы x1 < x2, y1 < y2
    void block(int x1, int y1, int x2, int y2, byte cl) {

        heap(vm, 0xf000);

        // Расчет инициирующей точки
        word  xc   = (x2>>1) - (x1>>1);     // Расстояние
        word  cc   = cl | (cl << 4);        // Сдвоенный цвет
        word  z    = _bank8(x1, y1);
        word  zc;

        // Коррекции, если не попадает
        if (x1 & 1) { z++; xc--; }
        if (x2 & 1) { xc++; }

        byte bank = inp(BANK_LO), zb; // Первичный банк

        // Построение линии сверху вниз
        for (int i = y1; i <= y2; i++) {

            // Выставить текущий банк
            outp(BANK_LO, bank);

            // Сохранение предыдущих указателей
            zc = z; zb = bank;

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
    }
};
