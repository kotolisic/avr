#include <avr/interrupt.h>

#include <textmode.cpp>
#include <sdcard.cpp>
#include <kb.cpp>

SDCard   sd;
TextMode tm;
KB       kb;

// Объявление прерывания
// ISR(INT0_vect) { brk; }

int main() {

    byte x = 0, y = 0;
    tm.start()->hide()->cls(0x17);
    
    for(;;) {

        if (kb.hit()) {

            tm.cursor(x, y)->printhex(kb.key(), 1)->printch(' ')->printfloat(TIMERW / 1000.0, 3);
            y++; if (y == 25) { x += 10; y = 0; }
        }
    }
}
