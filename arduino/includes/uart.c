#ifndef __UART_INCLUDE
#define __UART_INCLUDE

#ifndef UART_RATE
#define UART_RATE 9600L // Скорость по умолчанию
#endif

// Для работы UART требуется заранее объявить BAUD вот так, к примеру
#define BAUDRATE (F_CPU / (UART_RATE * 16)) - 1

// Инициализация USART в режиме 9600/8-N-1
void UART_init() {

    UBRR0L  = BAUDRATE;
    UBRR0H  = BAUDRATE >> 8;
    UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
    UCSR0C |= (1 << UMSEL01) | (1 << UCSZ01 ) | (1 << UCSZ00);
}

// Ввод-вывод UART
void         UART_put(unsigned char value) { while (!(UCSR0A & (1 << UDRE0))); UDR0 = value; }
unsigned int UART_get()                    { while (!(UCSR0A & (1 <<  RXC0))); return UDR0;  }

// Вывод строки
void UART_print(const char* s) {

    int i = 0;
    while (s[i]) UART_put(s[i++]);
}

// Печатать байт в виде HEX
void UART_hbyte(unsigned char b) {

    int h = (b & 0xf0) >> 4;
    int l = b & 0x0f;

    UART_put(h + (h < 10 ? '0' : '7'));
    UART_put(l + (l < 10 ? '0' : '7'));
}

#endif
