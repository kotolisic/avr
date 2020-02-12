#include <avr/io.h>
#include <util/delay.h>

#include <uart.c>
#include <twi.c>

int main (void)
{
    UART_init();
    I2C_init();

    while(1) {

        int bt = UART_get();
        if (bt == 10) {

            // I2C_start(); I2C_sla(0x68, 0); I2C_write(0); /* I2C_write(0x05); */ I2C_stop();

            // Write Start address
            I2C_start(); I2C_sla(0x68, 0); I2C_write(0); I2C_stop();

            for (int i = 0; i < 8; i++) {

                // Data
                I2C_start();
                I2C_sla(0x68, 1);
                int c1 = I2C_read();
                I2C_stop();

                UART_hbyte(c1);
                UART_put(' ');
            }


            UART_put(10);
        }
    }
}
