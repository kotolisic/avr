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

#include "appmenu.cpp"

AppMenu appmenu;

int main() {

    appmenu.selector();
}
