// https://github.com/mikaelpatel/Arduino-TWI/blob/master/src/Hardware/AVR/TWI.h

#ifndef TWI_RATE
#define TWI_RATE 100000 // 100 kHz
#endif

void I2C_init() {

    // Baud Rate
    TWBR = ((F_CPU / TWI_RATE) - 16) / 2;
    TWSR = 0; // Status  = 0, Prescaler = 1
    TWCR = 0; // Control = 0

    // Порты на выход
    DDRC  |= (1<<4) | (1<<5); // Output: SDA(4), SCL(5)
    PORTC |= (1<<4) | (1<<4); // SCL=0, SDA=0
}

// Ожидание ответа от шины
unsigned char I2C_wait() {

    // Ожидать, пока не будет 1
    while ((TWCR & _BV(TWINT)) == 0x00);
    return (TWSR & 0xF8);
}

// Запуск шины: Ответ 08h
unsigned char I2C_start() {

    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTA); // Отсылка START
    return I2C_wait();
}

// Остановка шины
void I2C_stop() {

    TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWSTO);         // Отсылка STOP
    for (char i = 0; i < 64; i++) asm volatile("nop");  // Задержка
    TWCR = 0;
}

// Отсылка байта. Должен быть ответ 28h
unsigned char I2C_write(unsigned char db) {

    TWDR = db;
    TWCR = _BV(TWEN) | _BV(TWINT);
    return I2C_wait();
}

// Запись байта, rw=0 (write) =1 (read). Ответ 18h (WRITE) или 40h (READ)
unsigned char I2C_sla(unsigned char data, unsigned char rw) {
    return I2C_write( (data << 1) | rw );
}

// Чтение данных: 0x50 (ACK) 0x58 (NACK)
unsigned char I2C_read() {

    TWCR = _BV(TWEN) | _BV(TWINT);
    I2C_wait();
    return TWDR;
}
