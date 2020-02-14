#include "avr.h"

// ---------------------------------------------------------------------
// Отображение видеобуфера на экран
// ---------------------------------------------------------------------

// Обновление экрана
void APP::update_screen() {

    if (ds_debugger)
         ds_update();
    else display_update();
}

// Обновить текст в (X, Y)
void APP::update_text_xy(int X, int Y) {

    if (videomode)
        return;

    int k;
    int addr = 0xF000 + 160*Y + 2*X;
    int ch   = sram[ addr + 0 ];
    int attr = sram[ addr + 1 ];

    for (int y = 0; y < 16; y++) {

        int ft = sram[0x10000 + 16*ch + y]; // ansi16[ch][y]; // sram
        for (int x = 0; x < 8; x++) {

            int cbit  = ft & (1 << (7 - x));
            int cursor = (cursor_x == X && cursor_y == Y) && (y >= 14) ? 1 : 0;
            int color = cbit ^ (flash & cursor) ? (attr & 0x0F) : (attr >> 4);

            int gb = sram[0xFFA0 + 2*color];
            int  r = sram[0xFFA1 + 2*color];

            // Вычисляется цвет
            color = ((gb & 0x0F) << 4) | ((gb & 0xF0) << 8) | ((r & 0x0F) << 20);

            for (int k = 0; k < 4; k++) {
                pset(2*(8*X + x) + k%2, 2*(16*Y + y) + k/2, color);
            }
        }
    }

    text_px = X;
    text_py = Y;
}

// 0xC000 - 0xFFFF Видеопамять
void APP::update_byte_scr(int addr) {

    // Область видеопамяти
    if (!ds_debugger) {

        // 0: TEXT MODE 80x25
        if (videomode == 0) {

            if ((addr >= 0xF000) && (addr < 0xFFA0)) {

                addr = (addr - 0xF000) >> 1;
                update_text_xy(text_px, text_py);
                update_text_xy(addr % 80, addr / 80);
            }
            // Обновить весь дисплей - меняется цвет в палитре
            else if ((addr >= 0xFFA0 && addr < 0xFFC0) ||  // Палитра
                     (addr >= 0x10000 && addr <= 0x10FFF)) // Знакоместо
                     { require_disp_update = 1; }
        }
        // 2: 256 x 240 x 2
        else if (videomode == 2 && (addr >= 0xC000) && (addr < 0xFC00)) {

            int cl;
            int cb = sram[addr];

            addr -= 0xC000;
            int Y = addr >> 6;
            int X = 4*(addr & 0x3F);

            // Нарисовать 4 точки (1 байт)
            for (int k = 0; k < 4; k++) {

                switch ((cb >> (6 - 2*k)) & 3) {

                    case 0: cl = 0x000000; break;
                    case 1: cl = 0xC00000; break;
                    case 2: cl = 0x00C000; break;
                    case 3: cl = 0x0060FF; break;
                }

                // 3x3 масштаб БК-0010
                for (int m = 0; m < 9; m++)
                pset((X + k)*3 + (m/3), Y*3 + (m%3), cl);
            }
        }
        // 1: 320 x 200 x 8
        else if (videomode == 1) {

            addr -= 0xc000;

            // Попадает точка
            if (addr >= 0 && addr < 64000) {

                uint cl = DOS_13[ sram[0xC000 + addr] ];

                int  X = addr % 320;
                int  Y = addr / 320;

                for (int m = 0; m < 16; m++) {
                    pset(4*X + (m>>2), 4*Y + (m&3), cl);
                }
            }
        }
        // 3: 320 x 200 x 4
        else if (videomode == 3) {

            addr -= 0x17000;
            if (addr >= 0 && addr < 32000) {

                int  X = (addr % 160) << 1;
                int  Y = (addr / 160);
                int  cb = sram[0x17000 + addr];

                // 2 Пикселя в байте
                for (int o = 0; o < 2; o++) {

                    uint cl = o ? cb & 15 : (cb >> 4);
                         cl = DOS_13[cl];

                    for (int m = 0; m < 16; m++) {
                        pset(4*(X + o) + (m>>2), 4*Y + (m&3), cl);
                    }
                }
            }
        }
    }
}

// Обновить весь экран
void APP::display_update() {

    int k, x, y;

    cls(0);

    // Видеорежим 80 x 25
    if (videomode == 0) {

        for (y = 0; y < 25; y++)
        for (x = 0; x < 80; x++)
            update_text_xy(x, y);
    }
    // Видеорежим 256 x 240 x 2
    else if (videomode == 2) {

        k = 0;
        for (y = 0; y < 720; y++) {
            for (x = 0; x < 768; x++) {

                int mx = x / 3, my = y / 3;
                int kx = mx & 3;
                int cb = sram[0xC000 + ((my*256 + mx) >> 2)];
                int cl;

                // 11.00.00.00 mx=0 >> 6 ==> 6 - 2*0 = 6
                // 00.11.00.00 mx=1 >> 4 ==> 6 - 2*1 = 4
                // 00.00.11.00 mx=2 >> 2 ==> 6 - 2*2 = 2
                // 00.00.00.11 mx=3 >> 0 ==> 6 - 2*3 = 0

                switch ((cb >> (6 - 2*kx)) & 3) {

                    case 0: cl = 0x000000; break;
                    case 1: cl = 0xC00000; break;
                    case 2: cl = 0x00C000; break;
                    case 3: cl = 0x0060FF; break;
                }

                ( (Uint32*)sdl_screen->pixels )[ width*y + x  ] = cl;
            }
        }
    }
    // Видеорежим 320 x 200 x 8
    else if (videomode == 1) {

        for (int x = 0; x < 1280; x++)
        for (int y = 0; y < 800; y++) {

            int X = x >> 2, Y = y >> 2;
            int cl = sram[X + Y*320 + 0xc000];
                cl = DOS_13[cl];

            pset(x, y, cl);
        }
    }
    // Видеорежим 320 x 200 x 4
    else if (videomode == 3) {

        for (int i = 0; i < 32000; i++) {
            update_byte_scr(0x17000 + i);
        }
    }
}

// ---------------------------------------------------------------------
// Процедуры для работы с выводом текста
// ---------------------------------------------------------------------

// Печать на экране Char
void APP::print3char(int col, int row, unsigned char ch, uint cl) {

    ch -= 0x20;

    col *= 4;
    row *= 8;

    for (int i = 0; i < 8; i++) {

        unsigned char hl = ansi3[ch >> 1][i];

        hl = (ch & 1 ? hl : hl>>4) & 0x0F;
        for (int j = 0; j < 4; j++) {

            int pcl = (hl & (1<<(3-j)) ? cl : 0);

            // Рисовать большой пиксель
            for (int k = 0; k < 4; k++)
                pset((j + col) + (k>>1), (i + row) + (k%2), pcl);
        }
    }
}

// Печать на экране Char
void APP::print16char(int col, int row, unsigned char ch, uint cl) {

    col *= 8;
    row *= 16;

    for (int i = 0; i < 16; i++) {

        unsigned char hl = ansi16[ch][i];
        for (int j = 0; j < 8; j++) {
            if (hl & (1<<(7-j)))
                pset(j + col, i + row, cl);
        }
    }
}

// Печать строки
void APP::print3(int col, int row, const char* s, uint cl) {

    int i = 0;
    while (s[i]) { print3char(col, row, s[i], cl); col++; i++; }
}

// Печать строки
void APP::print(int col, int row, const char* s, uint cl) {

    int i = 0;
    while (s[i]) { print16char(col, row, s[i], cl); col++; i++; }
}

// ---------------------------------------------------------------------
// Базовые методы вывода графики
// ---------------------------------------------------------------------

// Нарисовать точку
void APP::pset(int x, int y, uint color) {

    if (x >= 0 && y >= 0 && x < width && y < height) {
        ( (Uint32*)sdl_screen->pixels )[ x + width*y ] = color;
    }
}

// Очистить экран в цвет
void APP::cls(uint color) {

    for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
        pset(j, i, color);
}

// Обменять буфер
void APP::flip() {
    SDL_Flip(sdl_screen);
}
