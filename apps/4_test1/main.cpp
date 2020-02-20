#include <textmode.cpp>

TextMode tm;

int main() {

    tm.start()->hide();
    tm.cls(0x17);

    tm.frame(1,1,30,10,1);
    tm.cursor(3,2)->color(0x1F)->printutf8("Надо сделать редактор");    

    dword tp = 0;
    for(;;) {

        dword tl = TIMERW;

        if (tl != tp) {
            tm.cursor(3,4)->color(0x17)->printfloat(tl/1000.0, 3);
            tm.print("   ");
            tp = tl;
        }
    }
}
