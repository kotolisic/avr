#include "avr.h"

int main(int argc, char* argv[]) {

    APP avr(1280, 800, "AVR emulator");

    avr.config();
    avr.assign();
    
    if (argc > 1) { avr.loadfile(argv[1]); }
    
    avr.ds_update();
    avr.infinite();

    return 0;
}
