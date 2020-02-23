#include <avrio.c>

class DRAM {

protected:
public:

    // Записать байт
    void write(dword address, byte data) {

        outp(SDRAM_B0, address);
        outp(SDRAM_B1, address >> 8);
        outp(SDRAM_B2, address >> 16);
        outp(SDRAM_B3, address >> 24);
        outp(SDRAM_DATA, data);
        outp(SDRAM_CTRL, 1);

        // Ожидать завершения записи
        while (!(inp(SDRAM_CTRL) & 1));

        // Отключить запись
        outp(SDRAM_CTRL, 0);
    }

    // Записать байт
    byte read(dword address) {

        outp(SDRAM_B0, address);
        outp(SDRAM_B1, address >> 8);
        outp(SDRAM_B2, address >> 16);
        outp(SDRAM_B3, address >> 24);

        // Ожидать открытие строки
        while (!(inp(SDRAM_CTRL) & 1));

        // Прочитать готовые данные
        return inp(SDRAM_DATA);
    }
};
