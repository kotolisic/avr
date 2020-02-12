#include <avr/io.h>
#include <util/delay.h>

// Скорость обмена данными
#define BAUD 9600L

// Согласно заданной скорости подсчитываем значение для регистра UBRR
#define UBRRL_value ((F_CPU / (BAUD * 16)) - 1)

// Инициализация USART в режиме 9600/8-N-1
void init_USART() {

    // 16 бит UBRR
    UBRR0L  = UBRRL_value;
    UBRR0H  = UBRRL_value >> 8;

    // TXEN разрешить передачу и RXEN0 прием
    UCSR0B |= (1 << TXEN0) | (1 << RXEN0);

    // Устанавливаем формат 8 бит данных
    UCSR0C |= (1 << UMSEL01) | (1 << UCSZ01 ) | (1 << UCSZ00);
}

void send_UART(char value) {

    // Ожидаем когда очистится буфер передачи
    while (!( UCSR0A & (1 << UDRE0)));

    // Помещаем данные в буфер, начинаем передачу
    UDR0 = value;
}

unsigned int receive_UART(){

    while ((UCSR0A & (1 << RXC0)) == 0);
    return UDR0;
}

int main(void) {

    init_USART();

    while (1) {

        char m = receive_UART();
        send_UART(m);
    }
}
