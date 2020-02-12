#ifndef __TEXTMODE_INCLUDE
#define __TEXTMODE_INCLUDE

// Позиция курсора
char teletype_x,
     teletype_y,
     teletype_cl,
     cursor_has;

// Установка курсоа
void cursor(unsigned char x, unsigned char y) {

    teletype_x = x;
    teletype_y = y;

    if (cursor_has) {
        outp(4, x);
        outp(5, y);
    }
}

// Скрыть курсор
void cursor_hide() {

    outp(4, 80);
    cursor_has = 0;
}

// Показать курсор
void cursor_show() {

    cursor_has = 1;
    cursor(teletype_x, teletype_y);
}

// Очистка экрана
void cls(char cl) {

    teletype_x  = 0;
    teletype_y  = 0;
    teletype_cl = cl;
    cursor_has  = 1;

    char* vm = (char*) 0xF000;
    for (int i = 0; i < 2000; i++) {
        vm[2*i+0] = 0;
        vm[2*i+1] = cl;
    }
}

// Очистка для "графического" режима 80x50 точек
void gcls() {

    char* vm = (char*) 0xF000;
    for (int i = 0; i < 2000; i++) {
        vm[2*i+0] = 0xDF;
        vm[2*i+1] = 0x00;
    }
    cursor(80, 25);
}

// Псевдографическая цветная точка
void pset(char x, char y, char cl) {

    char* vm = (char*) 0xF000;
    char  y2 = y & 1;

    // Нельзя выезжать за пределы границ пространства и времени
    if (x >= 0 && x < 80 && y >= 0 && y < 50) {

        y >>= 1;
        unsigned int  ps   = 1 | ((x<<1) + (y<<5) + (y<<7));
        unsigned char attr = vm[ps];

        if (y2) {
            vm[ps] = (attr & 0x0F) | (cl << 4);
        } else {
            vm[ps] = (attr & 0xF0) | (cl & 0xF);
        }
    }
}

void color(unsigned char cl) {
    teletype_cl = cl;
}

// Установить курсор на новое место
void locate(unsigned char x, unsigned char y) {

    teletype_x = x;
    teletype_y = y;
    cursor(x, y);
}

// Печать символа не в телетайпе
void printc(char x, char y, char ch) {

    char* vm = (char*) 0xF000;
    int   z = (x<<1) + (y<<7) + (y<<5);

    vm[z]   = ch;
    vm[z+1] = teletype_cl;
}

// Печать в режиме телетайпа
void printch(char s) {

    int   i;
    char* vm = (char*) 0xF000;

    if (s == 10) {
        teletype_x = 80;
    } else {
        printc(teletype_x, teletype_y, s);
        teletype_x++;
    }

    if (teletype_x >= 80) {
        teletype_x = 0;
        teletype_y++;
    }

    // Скроллинг вверх
    if (teletype_y >= 25) {

        for (i = 0; i < 4000 - 160; i += 2) {
            vm[i]   = vm[i + 160];
            vm[i+1] = vm[i + 161];
        }

        // Очистка новой строки
        for (i = 4000 - 160; i < 4000; i += 2) {
            vm[i]   = ' ';
            vm[i+1] = teletype_cl;
        }

        teletype_y = 24;
    }

    cursor(teletype_x, teletype_y);
}

// Печать строки
void print(char* s) {
    while (*s) { printch(*s); s++; }
}

// Алиас
void println(char *s) { print(s); printch(10); }

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

// Обновить палитру
void palette(unsigned char id, unsigned char r, unsigned char g, unsigned char b) {

    char* vm = (char*) 0xFFA0;

    vm[2*id+0] = (b >> 4) | (g & 0xF0);
    vm[2*id+1] = (r >> 4);
}

// Рисовать фрейм
void frame(char x1, char y1, char x2, char y2, char density) {

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
}

// Рисование блока текста
void block(char x1, char y1, char x2, char y2, unsigned char cl) {

    teletype_cl = cl;
    for (int i = y1; i <= y2; i++)
    for (int j = x1; j <= x2; j++)
        printc(j, i, ' ');
}

#endif
