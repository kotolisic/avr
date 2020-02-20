#include <textmode.cpp>
#include <sdcard.cpp>
#include <kbd.cpp>

SDCard   sd;
TextMode tm;
KBD      kbd;

int main() {

    tm.start()->hide();
    tm.cls(0x17);

    tm.cursor(0,0);
    tm.printint(sd.init());

    tm.frame(1,1,30,10,1);
    tm.cursor(3,2)->color(0x1F)->printutf8("Надо сделать редактор");    

    dword tp = 0;
    for(;;) {

        dword tl = TIMERW;

        byte k = kbd.inkey();

        if (k) {
            tm.cursor(3, 5)->printch(k);
        }

        if (tl != tp) {
            tm.cursor(3,4)->color(0x17)->printfloat(tl/1000.0, 3);
            tm.print("   ");
            tp = tl;
        }
    }
}
