#ifndef __SPI_MODULE
#define __SPI_MODULE

// ---------------------------------------------------------------------
// SPI-протокол
// ---------------------------------------------------------------------

// Настроить 3=MOSI, 2=SS, 5=SCK на OUTPUT; 4=MISO на INPUT; Установка SCLK=0
void SPI_init() {

    //         SS       MOSI     SCK
    DDRB  |=  (1<<2) | (1<<3) | (1<<5);  // 2,3,5 OUTPUT
    DDRB  &= ~(1<<4);                    // 4     INPUT
    PORTB &= ~(1<<5);                    // SCLK  = 0
    PORTB |=  (1<<2);                    // SS    = 1

    // Включить SPI, Master, clock rate f_osc/128 (125 kHz), double_speed = 0
    SPCR  =  (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
    SPSR &= ~(1 << SPI2X);
}

// Общая функция трансмиттинга
unsigned char SPI_transmit(unsigned char data) {

    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

// Ввод-вывод на SPI
void SPI_put(unsigned char data) {        SPI_transmit(data); }
unsigned char SPI_get()          { return SPI_transmit(0xFF); }

// sig: 0=LOW, 1=HIGH
void SPI_ce(char sig) {

    if (sig) PORTB |=  (1<<2); // HIGH
        else PORTB &= ~(1<<2); // LOW
}

#endif
