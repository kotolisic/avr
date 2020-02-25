#include <textmode.cpp>
#include <kb.cpp>
#include <dram.cpp>

KB kb;
DRAM dram;
TextMode t;

#include "cpu86.cpp"

CPU86 cpu;

/*
 * Карта памяти
 * $0000 IVT
 * $0400 BDA
 * $7C00 Загрузчик
 *
 * Биос будет эмулироваться, честно говоря, пока что
 */

int main() {

    t.start();
    t.cls(0x07)->hide();


    cpu.step();

    for(;;);
}
