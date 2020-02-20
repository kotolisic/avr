#include <text.cpp>

int main() {

    TextMode canvas;

    canvas.start();
    canvas.cls(0x17);
    canvas.cursor(78,24);

    canvas.print("Hello, Borland");
    canvas.printfloat(1.5);

    for(;;);
}
