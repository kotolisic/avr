#include <avr/io.h>
#include <util/delay.h>

#define BAUD 9600L
#define UBRRL_value ((F_CPU / (BAUD * 16)) - 1)

#define cli asm volatile("cli")
#define sei asm volatile("sei")
#define nop asm volatile("nop")

// Инициализация USART в режиме 9600/8-N-1
void UART_init() {

    UBRR0L  = UBRRL_value;
    UBRR0H  = UBRRL_value >> 8;
    UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
    UCSR0C |= (1 << UMSEL01) | (1 << UCSZ01 ) | (1 << UCSZ00);
}

// Ввод-вывод UART
void UART_put(unsigned char value) { while (!( UCSR0A & (1 << UDRE0))); UDR0 = value; }
unsigned int UART_get() { while ((UCSR0A & (1 << RXC0)) == 0); return UDR0; }

// ---------------------------------------------------------------------
// SPI-протокол
// ---------------------------------------------------------------------

// Настроить 3=MOSI, 2=SS, 5=SCK на OUTPUT; 4=MISO на INPUT; Установка SCLK=0
void SPI_init() { DDRB |= (1<<5) | (1<<2) | (1<<3); DDRB &= ~(1<<4); PORTB &= ~(1<<5); }

// Отослать байт по SPI
void SPI_put(unsigned char data) {

    cli;
    for (char i = 0; i < 8; i++) {

        PORTB &= ~(1<<5); // SCK=0
        PORTB  = data & 0x80 ? (PORTB | (1<<2)) : (PORTB & (~(1<<2)));
        data <<= 1;
        PORTB |=  (1<<5); // SCK=1
    }

    // Удерживать SCK=1 несколько наносекунд
    nop; nop; nop; nop;
    PORTB &= ~(1<<5); // SCK=0
    sei;
}

// Прием данных из порта MISO
unsigned char SPI_get() {

    cli;
    unsigned char data = 0;

    // Отослать 0xFF
    PORTB |= (1<<2);
    for (char i = 0; i < 8; i++) {

        PORTB |= (1<<5); // SCK=1
        nop;
        nop;
        data <<= 1;

        // Принять 1, если есть на входе
        if (PINB & (1<<4)) data |= 1;
        PORTB &= ~(1<<5); // SCK=0
    }
    sei;
    return data;
}

int main(void) {

    SPI_init();
    UART_init();

    while (1) {

        char m = UART_get();
        SPI_put(m);
        UART_put(m);
    }
}
