#include "graphics.h"

// Установка графического режима
inline void screen3(char _enmouse) {

    g_cursor_x  = 0;
    g_cursor_y  = 0;
    g_cursor_cl = 15;
    enamouse    = _enmouse;

    outp(0x0D, 3);
}

inline void cursor(int x, int y) { g_cursor_x = x; g_cursor_y = y; }
inline void color(char cl) { g_cursor_cl = cl; }

// Вычисление адреса и банка
unsigned int _bank8(int x, int y) {

    unsigned int z = (y << 7) + (y << 5) + (x >> 1);
    outp(0, 8 | ((z >> 12) & 7));
    return z & 0xFFF;
}

// Рисование точки на экране
void _pset(int x, int y, char cl) {

    if (x < region_x1 || y < region_y1 || x > region_x2 || y > region_y2)
        return;

    char* vm = (char*) 0xf000;
    unsigned int z = _bank8(x, y);

    // Установка точки в ниббл
    cl   &= 15;
    vm[z] = x & 1 ? ((vm[z] & 0xF0) | cl) : ((vm[z] & 0x0F) | (cl << 4));
}

// Рисование блока. Необходимо, чтобы x1 < x2, y1 < y2
void _block(int x1, int y1, int x2, int y2, char cl) {

    // Расчет инициирующей точки
    char* vm   = (char*) 0xF000;
    int   xc   = (x2>>1) - (x1>>1);     // Расстояние
    int   cc   = cl | (cl << 4);        // Сдвоенный цвет
    int   z    = _bank8(x1, y1);
    int   zc;

    // Коррекции, если не попадает
    if (x1 & 1) { z++; xc--; }
    if (x2 & 1) { xc++; }

    char bank = inp(0), zb; // Первичный банк

    // Построение линии сверху вниз
    for (int i = y1; i <= y2; i++) {

        // Выставить текущий банк
        outp(0, bank);

        // Сохранение предыдущих указателей
        zc = z; zb = bank;

        // Рисование горизонтальной линии
        for (int j = 0; j < xc; j++) {

            vm[zc++] = cc;
            if (zc == 0x1000) { outp(0, ++bank); zc = 0; }
        }
        bank = zb;

        // К следующему Y++
        z += 160;

        // Перелистнуть страницу, если нужно
        if (z >= 0x1000) { z &= 0xFFF; bank++; }
    }

    // Дорисовать линии слева и справа
    if ( (x1 & 1)) for (int i = y1; i <= y2; i++) _pset(x1, i, cl);
    if (!(x2 & 1)) for (int i = y1; i <= y2; i++) _pset(x2, i, cl);
}

// Вернуть точку
char point(int x, int y) {

    // Точка находится в буфере мыши
    if (mouse_x <= x && x < mouse_x + 8 && mouse_y <= y && y < mouse_y + 14) {
        return mouse_bk[y - mouse_y][x - mouse_x];
    }

    unsigned char* vm = (unsigned char*) 0xF000;
    int z  = _bank8(x, y);

    return x & 1 ? vm[z] & 0x0F : (vm[z] >> 4);
}

// ---------------------------------------------------------------------
// Работа с мышью
// ---------------------------------------------------------------------

// Установка точки, с учетом мышки
void pset(int x, int y, char cl) {

    // Здесь находится область мыши
    if (enamouse && mouse_x <= x && x < mouse_x + 8 && mouse_y <= y && y < mouse_y + 14) {

        unsigned char xn = x - mouse_x;
        unsigned char yn = y - mouse_y;

        // Вычислить цвет точки
        char mc = mouse_icon[yn] >> (2*(7 - xn)) & 3;

        // Сохранить эту точку для point()
        mouse_bk[yn][xn] = cl;

        switch (mc) {

            case 1: cl = 7;  break;
            case 2: cl = 15; break;
            case 3: cl = 0;  break;
        }
    }

    _pset(x, y, cl);
}

// Блок с поддержкой мыши
void block(int x1, int y1, int x2, int y2, char cl) {

    int mr = mouse_x + 7;
    int mb = mouse_y + 14;

    // Вычисление возможности нарисовать окно
    if (x1 > x2 || y1 > y2 || x1 > region_x2 || y1 > region_y2 || x2 < region_x1 || y2 < region_y1)
        return;

    // Срезать по краям
    if (x1 <  region_x1 && x2 >= region_x1) x1 = region_x1;
    if (y1 <  region_y1 && y2 >= region_y1) y1 = region_y1;
    if (x1 <= region_x2 && x2 >  region_x2) x2 = region_x2;
    if (y1 <= region_y2 && y2 >  region_y2) y2 = region_y2;

    // Этот блок рисуется вне области мыши
    if (!enamouse || (x2 < mouse_x || y2 < mouse_y || x1 > mr || y1 > mb)) {
        _block(x1, y1, x2, y2, cl);

    } else {

        // Левый край мыши находится посередине блока
        if (x1 < mouse_x && mouse_x < x2) {

            _block(x1, y1, mouse_x - 1, y2, cl);
            x1 = mouse_x;
        }

        // Левый край блока находится за мышью
        if (x1 >= mouse_x) {

            int rx = (x2 < mr      ? x2 : mr);          // Ограничить блок справа
            int ry = (y2 < mouse_y ? y2 : mouse_y - 1); // Ограничить блок сверху

            // Рисовать блок сверху (всегда есть)
            _block(x1, y1, rx, ry, cl);

            // Обновление области мыши
            for (int i = mouse_y; i <= mb; i++)
            for (int j = mouse_x; j <= mr; j++)
                if (x1 <= j && j <= x2 && y1 <= i && i <= y2)
                    pset(j, i, cl);

            // Блок снизу может и не быть
            _block(x1, mb, rx, y2, cl);

            // Новый блок рисуется после мыши
            x1 = mr + 1;
        }

        // Дорисовать остатки блока справа, если остались
        _block(x1, y1, x2, y2, cl);
    }
}

// Jчистка всего экрана
void cls(char cl) {

    region_x1 = 0;
    region_y1 = 0;
    region_x2 = 319;
    region_y2 = 199;

    mouse_x = 160;
    mouse_y = 100;

    block(0, 0, region_x2, region_y2, cl);
}

// Установка мыши в новую точку
void mouse_move(int x, int y) {

    int i, j;

    char bgm1[14][8];
    char bgm2[14][8];

    int px = mouse_x,
        py = mouse_y;

    // Сохранение предыдущего рисунка фона
    for (i = 0; i < 14; i++)
    for (j = 0; j < 8; j++) {

        bgm1[i][j] = mouse_bk[i][j];        // Старые данные за мышью
        bgm2[i][j] = point(x + j, y + i);   // Новые данные у новой мыши
    }

    // Установка новой позиции мыши
    mouse_x = x;
    mouse_y = y;

    // Перерисовка старой области с предыдущей мышкой
    for (i = 0; i < 14; i++)
    for (j = 0; j < 8; j++) {
        pset(px + j, py + i, bgm1[i][j]);
        pset( x + j,  y + i, bgm2[i][j]);
    }
}
