#include <textmode.cpp>
#include <graphics.cpp>
#include <kb.cpp>
#include <dram.cpp>

KB       kb;
DRAM     dram;
TextMode t;
Graphics g;

int main() {

    t.start();
    t.cls(0x17);

}
