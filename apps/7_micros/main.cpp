// ---------------------------------------------------------------------
#include <textmode.cpp>
#include <graphics.cpp>
#include <kb.cpp>
#include <dram.cpp>

KB       kb;
DRAM     dram;
TextMode t;
Graphics g;
// ---------------------------------------------------------------------

int main() {

    g.start();
    g.cursor(8, 8)->print("Menu Entertainment");
    g.cursor(8, 32)->print("> Norton Commander");

    

}
