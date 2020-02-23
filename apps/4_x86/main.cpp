#include <textmode.cpp>
#include <kb.cpp>
#include <dram.cpp>

KB kb;
DRAM dram;
TextMode t;

/*
 * Карта памяти
 * $0000 IVT
 * $0400 BDA
 * $7C00 Загрузчик
 *
 * Биос будет эмулироваться, честно говоря, пока что
 */
dword ff(dword c) {
    return TIMERD - c;
}

int main() {

    t.start();
    t.cls(0x07)->hide();

    dword ptime = TIMERD; // pad = 0;

    int x = 0, y = 0;
    for (dword i = 0; i < (dword)64*1024*1024; i++) {

        dram.write(i, i);

        if ((i & 0xffff) == 0) {

            t.printc(x, y, '#');

            // Скорость записи в память
            t.cursor(0, 24)->printfloat( (float)i / ((float)(TIMERD - ptime) / 1000.0) / 1024.0 );
            t.print(" kbs    ");

            x++; if (x == 80) { x = 0; y++; }
        }
    }


    // Для начала скопировать программу bios в 64кб памяти
    // Запустить энтот биос
    // Загрузить по адресу $7C00 какой-нибудь сверхраспибиабузчик

    for(;;);
}
