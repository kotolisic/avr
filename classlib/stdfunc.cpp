class STDFunc {

protected:
public:

    // Печать числа -2147483647 .. 2147483647
    byte printint(long v) {

        char s[24];
        int  q, i = 0, cnt = 0;

        // Печать символа минус перед числом
        if (v < 0) { v = -v; printchar('-'); cnt = 1; }

        // Вычисление смещения
        do { q = v % 10; v /= 10; s[i++] = '0' + q; } while (v); i--;

        // Вывести число
        for (char k = 0; k <= i; k++) { printchar(s[i-k]); cnt++; }

        // Занимаемый размер символов
        return cnt;
    }

    // bits=1 (byte) =2 (word) =4 (dword)
    void printhex(unsigned long v, char bytes) {

        int sh = (bytes<<3) - 4;
        for (int i = 0; i < (bytes << 1); i++) {

            unsigned char m = (v >> (sh - 4*i)) & 0x0F;
            printchar(m < 10 ? '0' + m : '7' + m);
        }
    }

    // Печать float, n=2
    void printfloat(float x, char n) {

        if (x < 0) { x = -x; printchar('-'); }

        unsigned long i = (unsigned long) x;
        float f = x - i;

        // Печать целочисленного значения
        printint(i);
        printchar('.');

        // Печать остатка
        for (int k = 0; k < n; k++) {

            f *= 10;
            printchar(f + '0');
            f = f - (int)f;
        }
    }

    // Печать 2-х знаков после запятой
    void printfloat(float x) {  printfloat(x, 2); }
};
