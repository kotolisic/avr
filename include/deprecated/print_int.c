// @require 
// #include <print.c>
// #include <print3.c>

// Печать целого числа (int) -32768..32767
void print_int(unsigned char x, unsigned char y, int val, unsigned char cl) {

    set_cursor(x, y);

    unsigned int i = 9;
    volatile char dig[10]; 

    // Вывод отрицательного числа
    if (val < 0) { print_char('-', cl); val = -val; }

    do {
        
        int a = val % 10; val /= 10;
        dig[i--] = (a + '0'); 

    } while (val);

    // Печать целочисленного числа
    for (int k = i + 1; k < 10; k++) 
        print_char(dig[k], cl);
}