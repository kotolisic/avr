#include <avrio.c>

class DRAM {

protected:
public:

    // Записать байт
    byte read(dword address) {

        outp(SDRAM_B0, address);
        outp(SDRAM_B1, address >> 8);
        outp(SDRAM_B2, address >> 16);
        outp(SDRAM_B3, address >> 24);

        // Ожидать открытие строки
        while (!(inp(SDRAM_CTRL) & SDRAM_READY));

        // Прочитать готовые данные
        return inp(SDRAM_DATA);
    }

    // Записать байт
    void write(dword address, byte data) {

        outp(SDRAM_B0, address);
        outp(SDRAM_B1, address >> 8);
        outp(SDRAM_B2, address >> 16);
        outp(SDRAM_B3, address >> 24);

        outp(SDRAM_DATA, data);
        outp(SDRAM_CTRL, inp(SDRAM_CTRL) | SDRAM_WE); // WE=1

        // Ожидать завершения записи
        while (!(inp(SDRAM_CTRL) & SDRAM_READY));

        // Отключить запись
        outp(SDRAM_CTRL, inp(SDRAM_CTRL) & ~SDRAM_WE); // WE=0
    }

    // Чтение страницы целиком
    void read_page(dword page, byte dst[]) {

        outp(SDRAM_B1, page);
        outp(SDRAM_B2, page >> 8);
        outp(SDRAM_B3, page >> 16);

        // Считывание страницы
        for (int id = 0; id < 256; id++) {

            /* ADDR */ outp(SDRAM_B0, id);
            /* WAIT */ while (!(inp(SDRAM_CTRL) & SDRAM_READY));
            /* READ */ dst[id] = inp(SDRAM_DATA);
        }
    }

    // Запись страницы целиком
    void write_page(dword page, byte dst[]) {

        outp(SDRAM_B1, page);
        outp(SDRAM_B2, page >> 8);
        outp(SDRAM_B3, page >> 16);

        // Считывание страницы
        for (int id = 0; id < 256; id++) {

            /* ADDR */ outp(SDRAM_B0,   id);
            /* ADDR */ outp(SDRAM_DATA, dst[id]);
            /* ENA  */ outp(SDRAM_CTRL, inp(SDRAM_CTRL) |  SDRAM_WE); // WE=1
            /* WAIT */ while (!(inp(SDRAM_CTRL) & SDRAM_READY));
            /* DIS  */ outp(SDRAM_CTRL, inp(SDRAM_CTRL) & ~SDRAM_WE); // WE=0
        }
    }
};
