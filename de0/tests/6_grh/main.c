#include <avrio.c>
#include <kbd.c>
#include <graphics.c>
#include <line.c>
#include <print4.c>
#include <ui.c>

int main() {

    screen3();
    cls(3);

    window(10, 20, 200, 100, "Windows98");
    start_panel();

    for(;;) {
        
        //print4c(10, 10, getch(), 0);
    }
}
