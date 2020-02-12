// Документация тут

// https://eax.me/stm32-sdcard/
// http://elm-chan.org/docs/mmc/mmc_e.html

#include "spi.c"
#include "sd.h"

// Результат выполнения команды
unsigned char SD_command(unsigned char cmd, unsigned long arg) {

    int i = 0;
    unsigned char status;
    unsigned char crc = 0xFF;

    // Перевести в активный режим
    SPI_ce(0);

    // Подождать ответ в течении определенного времени
    while (i < SD_TIMEOUT_CNT) { status = SPI_get(); if (status == 0xff) break; i++; }

    // Ошибка
    if (i >= SD_TIMEOUT_CNT) { SD_error = SD_TimeoutError; return 0xff; }

    // Отсылка команды
    SPI_put(cmd | 0x40);

    // Отослать 32-х битную команду
    for (i = 24; i >= 0; i -= 8) SPI_put(arg >> i);

    // Отправка CRC
    if (cmd == SD_CMD0) crc = 0x95;  // CMD0 with arg 0
    if (cmd == SD_CMD8) crc = 0x87;  // CMD8 with arg 0x1AA

    SPI_put(crc);

    // Ожидать снятия флага BUSY
    for (i = 0; ((status = SPI_get()) & 0x80) && i != 0xFF; i++);

    return status;
}

// Расширенная команда
unsigned char SD_acmd(unsigned char cmd, unsigned long arg) {

    SD_command(SD_CMD55, 0);
    return SD_command(cmd, arg);
}

// Инициализация карты
void SD_init() {

    unsigned char i, status = 0xFF;
    unsigned long arg;

    // Вначале все вроде как ОК
    SD_error = SD_OK;
    SD_type  = SD_CARD_TYPE_ERR;

    // Отключить устройство
    SPI_ce(1);

    // Подать 80 тактов, которые переведут устройство в режим SPI
    for (i = 0; i < 10; i++) SPI_put(0xFF);

    // Сброс. Должен быть ответ 01h
    if (SD_command(SD_CMD0, 0) != R1_IDLE_STATE) {
        SD_error = SD_UnknownError; SPI_ce(1); return;
    }

    // Определить тип карты (SD1)
    if (SD_command(SD_CMD8, 0x1AA) & R1_ILLEGAL_COMMAND) {
        SD_type = SD_CARD_TYPE_SD1;

    } else {

        // Прочесть 4 байта, последний будет важный
        for (i = 0; i < 4; i++) status = SPI_get();

        // Неизвестный тип карты
        if (status != 0xAA) {
            SD_error = SD_UnknownCard; SPI_ce(1); return;
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
            SD_error = SD_AcmdError; SPI_ce(1); return;
        }
    }

    // Если SD2, читать OCR регистр для проверки SDHC карты
    if (SD_type == SD_CARD_TYPE_SD2) {

        // Проверка наличия байта в ответе CMD58 (должно быть 0)
        if (SD_command(SD_CMD58, 0)) {
            SD_error = SD_Unknown58CMD; SPI_ce(1); return;
        }

        // Прочесть ответ от карты и определить тип (SDHC если есть)
        if ((SPI_get() & 0xC0) == 0xC0) {
            SD_type = SD_CARD_TYPE_SDHC;
        }

        // Удалить остатки от OCR
        for (i = 0; i < 3; i++) SPI_get();
    }

    // Выключить чип
    SPI_ce(1);
}

// Читать блок 512 байт в память
char SD_read(unsigned long lba) {

    int i = 0;
    unsigned char status;

    // Кроме SDHC ничего не поддерживается
    if (SD_type != SD_CARD_TYPE_SDHC)
        return SD_UnsupportYet;

    // Отослать команду поиска блока
    if (SD_command(SD_CMD17, lba)) {
        SPI_ce(1); return SD_BlockSearchError;
    }

    // Ожидание ответа от SD
    while ((status = SPI_get()) == 0xFF)
        if (i++ > SD_TIMEOUT_CNT) {
            SPI_ce(1); return SD_TimeoutError;
        }

    // DATA_START_BLOCK = 0xFE
    if (status != 0xFE) {
        SPI_ce(1); return SD_BlockSearchError;
    }

    // Прочесть данные
    for (i = 0; i < 512; i++) SD_data[i] = SPI_get();

    SPI_ce(1);
    return SD_OK;
}

// Писать блок 512 байт в память
char SD_write(unsigned long lba) {

    int i = 0;
    unsigned char status;

    // Кроме SDHC ничего не поддерживается
    if (SD_type != SD_CARD_TYPE_SDHC)
        return SD_UnsupportYet;

    // Отослать команду поиска блока
    if (SD_command(SD_CMD24, lba)) {
        SPI_ce(1); return SD_BlockSearchError;
    }

    // DATA_START_BLOCK
    SPI_put(0xFE);

    // Запись данных
    for (int i = 0; i < 512; i++) SPI_put(SD_data[i]);

    // Dummy 16-bit CRC
    SPI_put(0xFF);
    SPI_put(0xFF);

    status = SPI_get();

    // Сверить результат
    if ((status & 0x1F) != 0x05) {
        SPI_ce(1); return SD_WriteError;
    }

    // Ожидание окончания программирования
    while ((status = SPI_get()) == 0xFF)
        if (i++ > SD_TIMEOUT_CNT) {
            SPI_ce(1); return SD_TimeoutError;
        }

    // response is r2 so get and check two bytes for nonzero
    if (SD_command(SD_CMD13, 0) || SPI_get()) {
        SPI_ce(1); return SD_WriteError;
    }

    SPI_ce(1);
    return SD_OK;
}
