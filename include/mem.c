#include <sd.c>

// Использование верхней памяти в качестве кеша
unsigned long memory_map[16];

// Инициализация памяти
void mm_init() {

    for (int i = 0; i < 16; i++)
        memory_map[i] = (i << 12);
}

// Сменить банк по адресу
void bank_switch(unsigned long address) {

    int i;
    unsigned char* vm = (unsigned char*) 0xF000;
    unsigned long lba;
    unsigned int  ab = (address >> 12) & 15;   // Кеш-линия
    unsigned long ba = (address & 0x000FF000); // Адрес 1Mb

    outp(0, ab);

    // Банк поменялся
    if (memory_map[ab] != ba) {

        // Записать старые данные, прочитать новые
        lba = 1 + (memory_map[ab] >> 9); for (i = 0; i < 8; i++) sd_write(lba++, vm + 512*i);
        lba = 1 + (ba >> 9);             for (i = 0; i < 8; i++) sd_read (lba++, vm + 512*i);

        memory_map[ab] = ba;
    }
}

// Реализация чтения/записи в длинную память
// rw = 1 (write), 0 (read)
void rwb(char rw, char buffer[], unsigned long address, unsigned int size) {

    int id;
    unsigned char* vm = (unsigned char*) 0xF000;

    bank_switch(address);

    // Записать/Читать байты данных из буфера
    for (id = 0; id < size; id++) {

        if (rw)
             vm[address & 0xFFF] = buffer[id]; // Write
        else buffer[id] = vm[address & 0xFFF]; // Read

        address++;

        // Достигнута граница банка, сменить банк
        if ((address & 0xFFF) == 0) bank_switch(address);
    }

    outp(0, 0);
}

// Чтение
void rb(char buffer[], unsigned long address, unsigned int size) { rwb(0, buffer, address, size); }
void wb(char buffer[], unsigned long address, unsigned int size) { rwb(1, buffer, address, size); }
