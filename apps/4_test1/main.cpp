#include <text.cpp>

int main() {

    TextMode cv;

    cv.start();
    cv.cls(0x17);

    cv.frame(1,1,30,10,1);
    cv.cursor(3,2)->color(0x1F)->print("Hello, Borland Pascaledron");

    word tp = 0;
    for(;;) {

        word tl = TIMERW;

        if (tl != tp) {
            cv.cursor(3,4)->color(0x17)->printfloat(tl / 1000.0, 3)->print("   ");
            tp = tl;
        }
    }
}
