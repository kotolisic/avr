#include <textmode.cpp>
#include <dram.cpp>

DRAM dram;
TextMode t;

// Простое заполнение данными и проверка скорости
void mem_unit1() {

    dword ptime = TIMERD;

    int x = 0, y = 0;
    dword cnt = 0;
    dword max = 64;

    // Заполнение данными
    for (dword i = 0; i < (dword)max*1024*1024; i++) {

        dram.write(i, i);

        if ((i & 0xffff) == 0) {

            t.printc(x, y, '#');

            // Скорость записи в память
            t.cursor(0, 24)->printfloat((1000.0 / 1024.0) * i / (float)(TIMERD - ptime));
            t.print(" kbs    ");

            t.printint((TIMERD - ptime)/1000);
            t.print(" s.   ");

            x++; if (x == 80) { x = 0; y++; }
        }
    }

    // Проверка целостности
    cnt = 0; y = 0; x = 0;
    for (dword i = 0; i < (dword)max*1024*1024; i++) { // 64

        byte dv = dram.read(i);

        // Тест ошибки
        if (dv != (i & 255)) {
            cnt++;
        }

        // Проверка на ошибки
        if ((i & 0xffff) == 0) {

            if (cnt > 0) {

                if (cnt < 10) cnt = '0' + cnt;
                else if (cnt >= 10) cnt = '!';

                t.color(0xCF)->printc(x, y, cnt);
            } else {
                t.color(0x0A)->printc(x, y, '#');
            }

            x++; if (x == 80) { x = 0; y++; }
            cnt = 0;
        }
    }
}

// Тест скорости страниц
void mem_unit2() {

    byte page[256]; // 256 байт - 1 страница

    dword ptime = TIMERD;

    for (dword p = 0; p < (dword)64*1024*4; p++) {

        dram.write_page(p, page);

        if ((p & 0xff) == 0) {

             t.cursor(0, 24)->printfloat((256000.0 / 1024.0) * p / (float)(TIMERD - ptime));
             t.print(" kbs    ");

             t.printint((TIMERD - ptime)/1000);
             t.print(" s.   ");
        }
    }

}

int main() {

    t.start();
    t.cls(0x07)->hide();

    mem_unit2();

    for(;;);
}
