#ifndef __SD_MODULE
#define __SD_MODULE

#define SPI_DAT         8
#define SPI_CMD         9
#define SPI_CMD_INIT    0
#define SPI_CMD_SEND    1
#define SPI_CMD_CE0     2
#define SPI_CMD_CE1     3
#define SD_TIMEOUT_CNT  4095
#define SD_BUSY         1
#define SD_TIMEOUT      2

char SD_error;
char SD_type;

enum SD_Commands {

    SD_CMD0     = 0,    // Сброс
    SD_CMD8     = 8,    // Проверка вольтажа SD2
    SD_CMD13    = 13,   // Проверка
    SD_CMD17    = 17,   // Чтение
    SD_CMD24    = 24,   // Запись
    SD_CMD55    = 55,   // ACMD
    SD_CMD58    = 58    // Чтение регистра OCR
};

enum SD_Results {

    R1_READY_STATE      = 0x00,
    R1_IDLE_STATE       = 0x01,
    R1_ILLEGAL_COMMAND  = 0x04
};

enum SD_Errors {

    SD_OK               = 0,
    SD_UnknownError     = 1,
    SD_TimeoutError     = 2,
    SD_UnknownCard      = 3,
    SD_AcmdError        = 4,
    SD_Unknown58CMD     = 5,
    SD_BlockSearchError = 6,
    SD_UnsupportYet     = 7,
    SD_WriteError       = 8,
    SD_WriteError2      = 9
};

enum SD_Types {

    SD_CARD_TYPE_ERR    = 255,
    SD_CARD_TYPE_SD1    = 1,
    SD_CARD_TYPE_SD2    = 2,
    SD_CARD_TYPE_SDHC   = 3
};

// Установки ошибки и притягивание к CE=1
unsigned char SD_set_error(char errorno) {

    SD_error = errorno;
    outp(SPI_CMD, SPI_CMD_CE1);
    return 0xFF;
}

// Принять байт SPI
inline unsigned char SPI_get() {

    outp(SPI_DAT, 0xFF);
    outp(SPI_CMD, SPI_CMD_SEND);
    return inp(SPI_DAT);
}

inline void SPI_put(unsigned char data) {

    outp(SPI_DAT, data);
    outp(SPI_CMD, SPI_CMD_SEND);
}

// Отсылка команды на SD-карту
unsigned char SD_command(unsigned char cmd, unsigned long arg) {

    int i;
    unsigned char crc, status = 0xFF, on_error = 1;

    // Включить устройство CE=0
    outp(SPI_CMD, SPI_CMD_CE0);

    // Принять байты, до тех пор, пока не будет 0xFF
    for (i = 0; i < SD_TIMEOUT_CNT; i++) {
        if (SPI_get() == 0xFF) {
            on_error = 0; break;
        }
    }

    // Ошибка ожидания ответа от SPI
    if (on_error) return SD_set_error(SD_TimeoutError);

    // Отсылка команды к SD
    outp(SPI_DAT, cmd | 0x40);
    outp(SPI_CMD, SPI_CMD_SEND);

    // Отослать 32-х битную команду
    for (i = 24; i >= 0; i -= 8) {

        outp(SPI_DAT, arg >> i);
        outp(SPI_CMD, SPI_CMD_SEND);
    }

    // Отправка CRC
    if (cmd == SD_CMD0) crc = 0x95;  // CMD0 with arg 0
    if (cmd == SD_CMD8) crc = 0x87;  // CMD8 with arg 0x1AA

    outp(SPI_DAT, crc);
    outp(SPI_CMD, SPI_CMD_SEND);

    // Ожидать снятия флага BUSY
    for (i = 0; i < 255; i++)
        if (((status = SPI_get()) & 0x80) == 0)
            break;

    return status;
}

// Расширенная команда
unsigned char SD_acmd(unsigned char cmd, unsigned long arg) {

    SD_command(SD_CMD55, 0);
    return SD_command(cmd, arg);
}

// Инициализация карты
unsigned char SD_init() {

    unsigned char status, i;
    unsigned long arg;

    SD_error = SD_OK;
    outp(SPI_CMD, SPI_CMD_INIT);

    // Тест на возможность войти в IDLE
    if (SD_command(SD_CMD0, 0) != R1_IDLE_STATE) {
        return SD_set_error(SD_UnknownError);
    }

    // Определить тип карты (SD1)
    if (SD_command(SD_CMD8, 0x1AA) & R1_ILLEGAL_COMMAND) {
        SD_type = SD_CARD_TYPE_SD1;

    } else {

        // Прочесть 4 байта, последний будет важный
        for (i = 0; i < 4; i++) status = SPI_get();

        // Неизвестный тип карты
        if (status != 0xAA) {
            return SD_set_error(SD_UnknownCard);
        }

        // Это тип карты SD2
        SD_type = SD_CARD_TYPE_SD2;
    }

    // Инициализация карты и отправка кода поддержки SDHC если SD2
    i   = 0;
    arg = (SD_type == SD_CARD_TYPE_SD2 ? 0x40000000 : 0);

    // Отсылка ACMD = 0x29. Отсылать и ждать, пока не придет корректный ответ
    while ((status = SD_acmd(0x29, arg)) != R1_READY_STATE) {

        // Если таймаут вышел
        if (i++ > SD_TIMEOUT_CNT) {
            return SD_set_error(SD_AcmdError);
        }
    }

    // Если SD2, читать OCR регистр для проверки SDHC карты
    if (SD_type == SD_CARD_TYPE_SD2) {

        // Проверка наличия байта в ответе CMD58 (должно быть 0)
        if (SD_command(SD_CMD58, 0)) {
            return SD_set_error(SD_Unknown58CMD);
        }

        // Прочесть ответ от карты и определить тип (SDHC если есть)
        if ((SPI_get() & 0xC0) == 0xC0) {
            SD_type = SD_CARD_TYPE_SDHC;
        }

        // Удалить остатки от OCR
        for (i = 0; i < 3; i++) SPI_get();
    }

    // Выключить чип
    outp(SPI_CMD, SPI_CMD_CE1);

    // Тип карты
    return SD_type;
}

// ---------------------------------------------------------------------
// Важные интерфейсные команды
// ---------------------------------------------------------------------

// Читать блок 512 байт в память: записыавается результат в SD_data
char sd_read(unsigned long lba, unsigned char SD_data[]) {

    int i = 0;
    unsigned char status;

    SD_error = SD_OK;
    if (inp(9) & SD_TIMEOUT) {
        SD_init();
    }

    // Кроме SDHC ничего не поддерживается
    if (SD_type != SD_CARD_TYPE_SDHC)
        return SD_set_error(SD_UnsupportYet);

    // Отослать команду поиска блока
    if (SD_command(SD_CMD17, lba)) {
        return SD_set_error(SD_BlockSearchError);
    }

    // Ожидание ответа от SD
    while ((status = SPI_get()) == 0xFF)
        if (i++ > SD_TIMEOUT_CNT)
            return SD_set_error(SD_TimeoutError);

    // DATA_START_BLOCK = 0xFE
    if (status != 0xFE) {
        return SD_set_error(SD_BlockSearchError);
    }

    // Прочесть данные
    for (i = 0; i < 512; i++) {
        SD_data[i] = SPI_get();
    }

    outp(SPI_CMD, SPI_CMD_CE1);
    return SD_OK;
}

// Писать блок 512 байт в память
char sd_write(unsigned long lba, unsigned char SD_data[]) {

    int i = 0;
    unsigned char status;

    SD_error = SD_OK;

    if (inp(9) & SD_TIMEOUT) {
        SD_init();
    }

    // Кроме SDHC ничего не поддерживается
    if (SD_type != SD_CARD_TYPE_SDHC)
        return SD_set_error(SD_UnsupportYet);

    // Отослать команду поиска блока
    if (SD_command(SD_CMD24, lba)) {
        return SD_set_error(SD_BlockSearchError);
    }

    // DATA_START_BLOCK
    SPI_put(0xFE);

    // Запись данных
    for (int i = 0; i < 512; i++) {
        SPI_put(SD_data[i]);
    }

    // Dummy 16-bit CRC
    SPI_put(0xFF);
    SPI_put(0xFF);

    status = SPI_get();

    // Сверить результат
    if ((status & 0x1F) != 0x05) {
        return SD_set_error(SD_WriteError);
    }

    // Ожидание окончания программирования
    while ((status = SPI_get()) == 0xFF)
        if (i++ > SD_TIMEOUT_CNT)
            return SD_set_error(SD_TimeoutError);

    // response is r2 so get and check two bytes for nonzero
    if (SD_command(SD_CMD13, 0) || SPI_get())
        return SD_set_error(SD_WriteError2);

    outp(SPI_CMD, SPI_CMD_CE1);
    return SD_OK;
}

#endif
