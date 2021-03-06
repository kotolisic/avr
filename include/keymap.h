#ifndef KEYMAP_HEADER
#define KEYMAP_HEADER

/*
 * 1  ^  TOP        60
 * 2  -> RIGHT      61
 * 3  v  DOWN       62
 * 4  <- LEFT       64
 * 5  Home          6E
 * 6  End           6F
 * 8  BkSpc         0E
 * 9  Tab           0F
 * 10 Enter         1C
 * ----------------------------------
 * 11-F1  | 14-F4  | 16-F7  | 19-F10
 * 12-F2  | 15-F5  | 17-F8  | 20-F11
 * 13-F3  | 16-F6  | 18-F9  | 21-F12
 * ----------------------------------
 * 22 PgUp          ~55
 * 23 PgDn          ~56
 * 24 Del           ~59
 * 25 Ins           ~5A
 * 26 NumLock        45
 * 27 Esc            01
 * 28 Win           ~5B
 * */

 // @todo добавить спецклавиши (и их трансляции)

// Перевод XT -> ASCII
static const unsigned char keymap[2][0x80] =
{
    {
    //   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
         0,  27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  8,   9,   // 0x
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  10,  0,  'a', 's',  // 1x
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',  0,  0x5C, 'z', 'x', 'c', 'v', // 2x
        'b', 'n', 'm', ',', '.', '/',  0,  '*',  0,  ' ',  0,  11,  12,  13,  14,  15,   // 3x
        16,  17,  18,  19,  20,  26,   0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',  // 4x
        '2', '3', '0', '.',  0,  22,  23,  21,  22,  24,  25,  28,   0,   0,   0,   0,   // 5x
         1,   2,   3,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   6,   // 6x
         0,  26,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 7x
    },
    {
   //    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
         0,  27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  8,   9,   // 0x
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  10,  0,  'A', 'S',  // 1x
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  0,  '|', 'Z', 'X', 'C', 'V',  // 2x
        'B', 'N', 'M', '<', '>', '?',  0,   0,   0,  ' ',  0,  0,    1,   2,   3,   4,   // 3x
        16,  17,  18,  19,  20,  26,   0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',  // 4x
        '2', '3', '0', '.',  0,  22,  23,  21,  22,  24,  25,  28,   0,   0,   0,   0,   // 5x
         1,   2,   3,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   6,   // 6x
         0,  26,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 7x
    }
};
#endif
