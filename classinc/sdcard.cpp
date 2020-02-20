#include "sdcard.h"

class SDCard {

protected:

    byte SD_error;
    byte SD_type;

public:

    // Установки ошибки и притягивание к CE=1
    byte set_error(byte errorno) {

        SD_error = errorno;
        outp(SPI_CMD, SPI_CMD_CE1);
        return 0xFF;
    }

    // Отсылка команды на SD-карту
    byte command(byte cmd, dword arg) {

        int i;
        byte crc, status = 0xFF, on_error = 1;

        // Включить устройство CE=0
        outp(SPI_CMD, SPI_CMD_CE0);

        // Принять байты, до тех пор, пока не будет 0xFF
        for (i = 0; i < SD_TIMEOUT_CNT; i++) {
            if (SPI_get() == 0xFF) {
                on_error = 0;
                break;
            }
        }

        // Ошибка ожидания ответа от SPI
        if (on_error) {
            return set_error(SD_TimeoutError);
        }

        // Отсылка команды к SD
        outp(SPI_DATA, cmd | 0x40);
        outp(SPI_CMD,  SPI_CMD_SEND);

        // Отослать 32-х битную команду
        for (i = 24; i >= 0; i -= 8) {

            outp(SPI_DATA, arg >> i);
            outp(SPI_CMD,  SPI_CMD_SEND);
        }

        // Отправка CRC
        if (cmd == SD_CMD0) crc = 0x95;  // CMD0 with arg 0
        if (cmd == SD_CMD8) crc = 0x87;  // CMD8 with arg 0x1AA

        outp(SPI_DATA, crc);
        outp(SPI_CMD,  SPI_CMD_SEND);

        // Ожидать снятия флага BUSY
        for (i = 0; i < 255; i++)
            if (((status = SPI_get()) & 0x80) == 0)
                break;

        return status;
    }

    // Расширенная команда
    byte acmd(byte cmd, dword arg) {

        command(SD_CMD55, 0);
        return command(cmd, arg);
    }

    // Инициализация карты
    byte init() {

        byte  status, i;
        dword arg;

        SD_error = SD_OK;
        outp(SPI_CMD, SPI_CMD_INIT);

        // Тест на возможность войти в IDLE
        if (command(SD_CMD0, 0) != R1_IDLE_STATE) {
            return set_error(SD_UnknownError);
        }

        // Определить тип карты (SD1)
        if (command(SD_CMD8, 0x1AA) & R1_ILLEGAL_COMMAND) {
            SD_type = SD_CARD_TYPE_SD1;

        } else {

            // Прочесть 4 байта, последний будет важный
            for (i = 0; i < 4; i++) status = SPI_get();

            // Неизвестный тип карты
            if (status != 0xAA) {
                return set_error(SD_UnknownCard);
            }

            // Это тип карты SD2
            SD_type = SD_CARD_TYPE_SD2;
        }

        // Инициализация карты и отправка кода поддержки SDHC если SD2
        i   = 0;
        arg = (SD_type == SD_CARD_TYPE_SD2 ? 0x40000000 : 0);

        // Отсылка ACMD = 0x29. Отсылать и ждать, пока не придет корректный ответ
        while ((status = acmd(0x29, arg)) != R1_READY_STATE) {

            // Если таймаут вышел
            if (i++ > SD_TIMEOUT_CNT) {
                return set_error(SD_AcmdError);
            }
        }

        // Если SD2, читать OCR регистр для проверки SDHC карты
        if (SD_type == SD_CARD_TYPE_SD2) {

            // Проверка наличия байта в ответе CMD58 (должно быть 0)
            if (command(SD_CMD58, 0)) {
                return set_error(SD_Unknown58CMD);
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

    // Читать блок 512 байт в память: записыавается результат в SD_data
    byte read(dword lba, byte SD_data[]) {

        int i = 0;
        unsigned char status;

        SD_error = SD_OK;

        // В случае истечения таймаута ожидания
        if (inp(SPI_STATUS) & SD_TIMEOUT) {
            init();
        }

        // Кроме SDHC ничего не поддерживается
        if (SD_type != SD_CARD_TYPE_SDHC)
            return set_error(SD_UnsupportYet);

        // Отослать команду поиска блока
        if (command(SD_CMD17, lba)) {
            return set_error(SD_BlockSearchError);
        }

        // Ожидание ответа от SD
        while ((status = SPI_get()) == 0xFF)
            if (i++ > SD_TIMEOUT_CNT)
                return set_error(SD_TimeoutError);

        // DATA_START_BLOCK = 0xFE
        if (status != 0xFE) {
            return set_error(SD_BlockSearchError);
        }

        // Прочесть данные
        for (i = 0; i < 512; i++) {
            SD_data[i] = SPI_get();
        }

        outp(SPI_CMD, SPI_CMD_CE1);
        return SD_OK;
    }

    // Писать блок 512 байт в память
    byte sd_write(dword lba, byte SD_data[]) {

        int i = 0;
        byte status;

        SD_error = SD_OK;

        // В случае истечения таймаута ожидания
        if (inp(SPI_STATUS) & SD_TIMEOUT) {
            init();
        }

        // Кроме SDHC ничего не поддерживается
        if (SD_type != SD_CARD_TYPE_SDHC)
            return set_error(SD_UnsupportYet);

        // Отослать команду поиска блока
        if (command(SD_CMD24, lba)) {
            return set_error(SD_BlockSearchError);
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
            return set_error(SD_WriteError);
        }

        // Ожидание окончания программирования
        while ((status = SPI_get()) == 0xFF)
            if (i++ > SD_TIMEOUT_CNT)
                return set_error(SD_TimeoutError);

        // response is r2 so get and check two bytes for nonzero
        if (command(SD_CMD13, 0) || SPI_get())
            return set_error(SD_WriteError2);

        outp(SPI_CMD, SPI_CMD_CE1);
        return SD_OK;
    }
};
