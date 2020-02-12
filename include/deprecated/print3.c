#include <ansi3.h>

unsigned char cursor3x, cursor3y;

void set_cursor(int x, int y) {
    cursor3x = x;
    cursor3y = y;
}

// Печать одного символа 4x8 на экране
void print_char(unsigned char ch, char cl) {

    unsigned char* m = (unsigned char*)0xc000;
    int ptr = (cursor3y<<9) + (cursor3x);

    cl &= 3;
    ch -= 0x20;

    for (int i = 0; i < 8; i++) {

        unsigned char hl = ansi3[ch >> 1][i];

        hl = (ch & 1 ? hl : hl>>4) & 0x0F;
        hl = (hl & 1 ? cl : 0) | 
             (hl & 2 ? cl<<2 : 0) |
             (hl & 4 ? cl<<4 : 0) |
             (hl & 8 ? cl<<6 : 0);

        m[ptr] = hl;
        ptr += 0x40;
    }

    cursor3x++;
    if (cursor3x == 64) {
        cursor3x = 0; 
        cursor3y++; 
        // @todo scroll up
    }
}

// Печать в телетайпе
void prints(char*s, char cl) {
    while (*s) { print_char(*s, cl); s++; }
}

// Пропечатка строки
void print(int x, int y, char* s, char cl) {

    set_cursor(x, y);
    prints(s, cl);    
}
