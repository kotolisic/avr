#define I2C_SDA     (1 << 4)
#define I2C_SCL     (1 << 5)

// Software I2C (задержка)
void I2C_delay() { for (int _dly = 0; _dly < 24; _dly++) asm volatile("nop"); }

// Инициализировать порты
void I2C_init() {

    DDRC  |= I2C_SDA | I2C_SCL; // Output: SDA(4), SCL(5)
    PORTC |= I2C_SDA | I2C_SCL; // SCL=1, SDA=1
    I2C_delay();
}

// Послать сигнал START
void I2C_start() {

    I2C_init();
    PORTC &= ~I2C_SDA; // SDA=0, SCL=1
    I2C_delay();
    PORTC &= ~I2C_SCL; // SDA=0, SCL=0
    I2C_delay();
}

// Послать сигнал STOP
void I2C_stop() {

    DDRC  |=   I2C_SDA | I2C_SCL;
    PORTC &= ~(I2C_SDA | I2C_SCL); // SDA=0, SCL=0
    I2C_delay();
    PORTC |= I2C_SDA; // SDA=1, SCL=0
    I2C_delay();
    PORTC |= I2C_SCL; // SDA=1, SCL=1
}

// Перевести в режим ожидания ACK. Проверить на валидность
char I2C_ack() {

    char valid = 0;

    PORTC &= ~(I2C_SDA | I2C_SCL); // SDA=0, SCL=0
    DDRC  &= ~(I2C_SDA);           // SDA (Input)

    I2C_delay();
    PORTC |= I2C_SCL; // SCL=1

    // Если вернулся 0, то валидно
    I2C_delay();
    valid = (PINC & I2C_SDA) ? 0 : 1;

    // SCL=0
    PORTC &= ~I2C_SCL;
    I2C_delay();

    return valid;
}

// Отослать байт (и проверить ACK)
void I2C_write(unsigned char data) {

    PORTC &= ~(I2C_SDA | I2C_SCL);
    DDRC  |=  (I2C_SDA | I2C_SCL);

    for (int i = 0; i < 8; i++) {

        // SCL=0
        PORTC &= ~I2C_SCL; I2C_delay();

        // Установить SDA
        if (data & 0x80)
             PORTC |=  I2C_SDA; // 1
        else PORTC &= ~I2C_SDA; // 0

        // Сместить на 1 бит (т.к. MSB)
        data <<= 1;

        // SCL=1
        PORTC |= I2C_SCL; I2C_delay();
    }

    I2C_ack();
}

// Принять байт
unsigned char I2C_read() {

    PORTC &= ~(I2C_SDA | I2C_SCL);
    DDRC  |=  (I2C_SCL); // OUT
    DDRC  &= ~(I2C_SDA); // IN

    unsigned char data = 0x00;
    for (int i = 0; i < 8; i++) {

        PORTC &= ~I2C_SCL; // SCL=0
        I2C_delay();

        PORTC |= I2C_SCL;  // SCL=1
        I2C_delay();

        // Установить SDA
        data <<= 1; if (PINC & I2C_SDA) data |= 1;
        I2C_delay();
    }

    I2C_ack();
    return data;
}
