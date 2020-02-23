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

int main() {

    t.start();
    t.cls(0x07);

    //brk;
    byte dv;

    for (int i = 0; i < 256; i++) {
        dram.write(i, i);
    }

    for (int i = 0; i < 256; i++) {

        dv = dram.read(i);
        t.printhex(dv, 1);
        t.printch(' ');
    }

    

    // Для начала скопировать программу bios в 64кб памяти
    // Запустить энтот биос
    // Загрузить по адресу $7C00 какой-нибудь сверхраспибиабузчик

    for(;;);
}
