#ifndef __AVRIO_HFILE
#define __AVRIO_HFILE

// Ссылка на пустой адрес
#define NULL    ((void*)0)
#define brk     asm volatile("sleep"); // break

// Базовые типы данных
#define byte        unsigned char
#define uint        unsigned int
#define word        unsigned int
#define ulong       unsigned long
#define dword       unsigned long

// Описания всех портов
enum PortsID {

    BANK_LO         = 0x00, // RW
    BANK_HI         = 0x01, // RW

    KB_DATA         = 0x02, // R
    KB_HIT          = 0x03, // R

    CURSOR_X        = 0x04, // RW
    CURSOR_Y        = 0x05, // RW

    TIMER_LO        = 0x06, // R
    TIMER_HI        = 0x07, // R
    TIMER_HI2       = 0x0F, // R
    TIMER_HI3       = 0x10, // R

    SPI_DATA        = 0x08, // W
    SPI_CMD         = 0x09, // W
    SPI_STATUS      = 0x09, // R

    MOUSE_X_LO      = 0x0A, // R
    MOUSE_Y_LO      = 0x0B, // R
    MOUSE_STATUS    = 0x0C, // R
    MOUSE_X_HI      = 0x0E, // R

    VIDEOMODE       = 0x0D, // RW

    // Управление SDRAM
    SDRAM_B0        = 0x10, //  7:0
    SDRAM_B1        = 0x11, // 15:8
    SDRAM_B2        = 0x12, // 23:16
    SDRAM_B3        = 0x13, // 31:24
    SDRAM_DATA      = 0x14, // RW
    SDRAM_CTRL      = 0x15, // R  Status [0=Ready], W Control [0=WE]
};

// Список видеорежимов
enum VideoModes {

    VM_80x25        = 0,
    VM_320x200x8    = 1,
    VM_320x240x2    = 2,
    VM_320x200x4    = 3
};

enum SPI_Commands {

    SPI_CMD_INIT    = 0,
    SPI_CMD_SEND    = 1,
    SPI_CMD_CE0     = 2,
    SPI_CMD_CE1     = 3
};

enum SDRAM_Status {

    SDRAM_WE        = 1,
    SDRAM_READY     = 2
};

// Значение таймера [15:0] или [31:0]
#define TIMERW ((word) inp(TIMER_LO) + ((word) inp(TIMER_HI)<<8))
#define TIMERD ((dword)inp(TIMER_LO) + ((dword)inp(TIMER_HI)<<8) + ((dword)inp(TIMER_HI2)<<16))

// Чтение из порта
inline byte inp(int port) {
    return ((volatile byte*)0x20)[port];
}

// Запись в порт
inline void outp(int port, unsigned char val) {
    ((volatile unsigned char*)0x20)[port] = val;
}

// Положение мыши
inline int get_mouse_x()    { return inp(0xA) | (inp(0xE) << 8); }
inline int get_mouse_y()    { return inp(0xB); }
inline int get_mouse_st()   { return inp(0xC); }

// Принять байт SPI
inline byte SPI_get() {

    outp(SPI_DATA, 0xFF);
    outp(SPI_CMD,  SPI_CMD_SEND);
    return inp(SPI_DATA);
}

// Отправить байт SPI
inline void SPI_put(byte data) {

    outp(SPI_DATA, data);
    outp(SPI_CMD,  SPI_CMD_SEND);
}

// Объявление указателя на память (имя x, адрес a)
#define heap(x, a)  byte* x = (byte*) a
#define bank(x)     outp(BANK_LO, x)

#endif
