#include <avr/interrupt.h>

#include <graphics.cpp>
#include <kb.cpp>

Graphics g;
KB       kb;

int main() {

    g.start();
    g.block(0,0,320,200,3);

    for(;;);
}
