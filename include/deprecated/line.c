// #define fastpset false

// Рисование линии
void line
(
    unsigned char x1, 
    unsigned char y1, 
    unsigned char x2, 
    unsigned char y2, 
    unsigned char cl
) {

    #ifdef fastpset
    unsigned char* vm = (unsigned char*) 0xc000;
    #endif

    if (y2 < y1) {
        x1 ^= x2; x2 ^= x1; x1 ^= x2;
        y1 ^= y2; y2 ^= y1; y1 ^= y2; 
    }

    int deltax = x2 > x1 ? x2 - x1 : x1 - x2;
    int deltay = y2 - y1;
    int signx  = x1 < x2 ? 1 : -1;

    int error2;
    int error = deltax - deltay;

    // Предварительные вычисления
    #ifdef fastpset
    int   a = (y1<<6) + (x1>>2);
    unsigned char b = (3^(x1 & 3)) << 1;
    unsigned char c = 3 << b, d = (cl & 3)<<b;
    #endif

    while (x1 != x2 || y1 != y2)
    {
        #ifdef fastpset
        vm[a] = (vm[a] & ~c) | d;
        #else 
        pset(x1, y1, cl);
        #endif

        error2 = error * 2;

        if (error2 > -deltay) {
            error -= deltay;
            x1 += signx;

            // Инкрементальный сдвиг влево или вправо
            #ifdef fastpset
            if (signx > 0) { c >>= 2; if (c) d >>= 2; else { c = 0xc0; d <<= 6; a++; } }
                      else { c <<= 2; if (c) d <<= 2; else { c = 0x03; d >>= 6; a--; } }
            #endif
        }

        if (error2 < deltax) {
            error += deltax;
            #ifdef fastpset
            a += 0x40;
            #endif
            y1 += 1;
        }
    }

    // Установка финальнай точки
    #ifdef fastpset
    vm[a] = (vm[a] & ~c) | d;
    #else
    pset(x1, y1, cl);
    #endif
};
