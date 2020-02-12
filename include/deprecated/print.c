#ifndef PRINTC_INC
#define PRINTC_INC

#include "fonts.h"

int teletype_x, teletype_y;

// Поставить курсор
void set_cursor(int x, int y) {
    teletype_x = x;
    teletype_y = y;
}

// Печать одного символа
void print_char(char ch, char cl) {

    int x = teletype_x, y = teletype_y;
    unsigned char* m = (unsigned char*)0xc000;
    int ptr = (y<<9) + (x<<1);

    for (unsigned char i = 0; i < 8; i++) {

        unsigned char hl = font8x8_basic[ch - 0x20][i];
        unsigned char a = hl & 15, b = hl >> 4;

        a = (a&8 ?  cl : 0) | (a&4 ? (cl<<2) : 0) | (a&2 ? (cl<<4) : 0) | (a&1 ? (cl<<6) : 0);
        b = (b&8 ?  cl : 0) | (b&4 ? (cl<<2) : 0) | (b&2 ? (cl<<4) : 0) | (b&1 ? (cl<<6) : 0);

        m[ptr+0] = a;
        m[ptr+1] = b;

        ptr += 0x40;
    }

    teletype_x++;
    if (teletype_x == 32) {
        teletype_x = 0;
        teletype_y++;
    }
}

// Печать в телетайпе
void prints(char*s, char cl) {
    while (*s) { print_char(*s, cl); s++; }
}

// Печать строки
void print(unsigned char x, unsigned char y, char* str, unsigned char cl) {

    set_cursor(x, y);
    while (*str) {

        print_char(*str, cl);
        str++;
    }
}

#endif
