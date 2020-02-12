#ifndef __KBD_INCLUDE
#define __KBD_INCLUDE

#include <keymap.h>

unsigned char key_shift, key_spec;

void kb_init() {

    key_shift = 0;
    key_spec  = 0;
}

// Чтение из порта нового символа
unsigned char inkey() {

    unsigned k_id = 0;

    // Ожидание нажатия клавиши
    if (inp(3) & 1) {

        unsigned char xt = inp(2);

        if /*CS*/ (xt == 0x2A) key_shift = 1;
        else if   (xt == 0xAA) key_shift = 0;

        // @todo CtrlKey

        // Что-то нажато (не отжато)
        if ((xt & 0x80) == 0) {

            // Обработка специальных клавиш
            if (key_spec) {

                switch (xt) {

                    case 0x48: xt = 0x60; break; // Up
                    case 0x4D: xt = 0x61; break; // Rt
                    case 0x50: xt = 0x62; break; // Dn
                    case 0x4B: xt = 0x64; break; // Lf
                    case 0x47: xt = 0x6E; break; // Home
                    case 0x4F: xt = 0x6F; break; // End
                    case 0x49: xt = 0x55; break; // PgUp
                    case 0x51: xt = 0x56; break; // PgDn
                    case 0x52: xt = 0x5A; break; // Ins
                    case 0x53: xt = 0x59; break; // Del
                    case 0x5C: xt = 0x5B; break; // Right Win, 5B Left Win
                    // case 0x1C: xt = 0x1C; break; // Enter
                }
            }

            k_id = keymap[ key_shift ][xt];
        }

        // Специальная клавиша
        if (xt == 0xE0) { key_spec = 1; return 0; }

        // После спец-клавиши всегда идет обычная
        key_spec = 0;
    }

    return k_id;
}

// Ожидание получения нажатия клавиши
unsigned char getch() {

    unsigned char k;
    while ((k = inkey()) == 0);
    return k;
}

#endif
