#include <textmode.cpp>
#include <kb.cpp>

KB kb;
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

    outp(SDRAM_B0, 0);
    outp(SDRAM_B1, 0);
    outp(SDRAM_B2, 0);
    outp(SDRAM_B3, 0);
    while (!(inp(SDRAM_CTRL) & 1));

    for (int i = 0; i < 256; i++) {

        outp(SDRAM_DATA, i);
        outp(SDRAM_B0, i);
        outp(SDRAM_CTRL, 1);
        while (!(inp(SDRAM_CTRL) & 1));
        outp(SDRAM_CTRL, 0);
    }

    for (int i = 0; i < 256; i++) {

        outp(SDRAM_B0, i);
        while (!(inp(SDRAM_CTRL) & 1)); // Ждать ответа ready=1

        dv = inp(SDRAM_DATA);
        t.printhex(dv, 1);
        t.printch(' ');
    }

    

    // Для начала скопировать программу bios в 64кб памяти
    // Запустить энтот биос
    // Загрузить по адресу $7C00 какой-нибудь сверхраспибиабузчик

    for(;;);
}
