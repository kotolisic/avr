#include <text.cpp>

int main() {

    TextMode cv;

    cv.start();
    cv.cls(0x17);

    cv.frame(1,1,30,10,1);
    cv.cursor(3,2)->print("Hello, Borland Pascaledron");

    for(;;);
}
