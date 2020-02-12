#include <avr/io.h>
#include <util/delay.h>
 
#define BLINK_DELAY_MS 250

int main (void)
{
    /* Установка пина 5 на PORTB как OUTPUT */
    DDRB |= _BV(DDB5);

    while(1) {

        // Включить пин 5 на высокий уровень (HIGH)
        PORTB |= _BV(PORTB5);

        // Задержка в миллисекундах
        _delay_ms(BLINK_DELAY_MS);

        // Выключить (LOW) пин 5, индикатор выключается
        PORTB &= ~_BV(PORTB5);
        _delay_ms(BLINK_DELAY_MS);
    }
}
