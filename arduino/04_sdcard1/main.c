#include <avr/io.h>
#include <util/delay.h>

#define BAUD 9600L

#include <uart.c>
#include <sd.c>

int main (void)
{
    SPI_init();
    UART_init();

    while(1) {

        int bt = UART_get();

        // Команда чтения
        if (bt == 'r') {

            UART_print("Device\n");

            // Инициализация
            SD_init();

            // Сектор начинается с 0
            int status = SD_read(1);

            // Ошибка?
            if (status) {
                 UART_print("READ FAIL "); UART_hbyte(status); UART_hbyte(10);
            }
            // Нормально
            else {

                for (int i = 0; i < 512; i++) {

                    UART_hbyte(SD_data[i]); UART_put(' ');
                    if ((i % 32) == 31) UART_put(10);

                    SD_data[i] += i;
                }

                // Инкремент для проверки
                if (status = SD_write(1)) {
                    UART_print("WRITE FAIL "); UART_hbyte(status); UART_hbyte(10);
                }
            }

        }
    }
}
