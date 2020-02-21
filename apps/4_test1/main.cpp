#include <avr/interrupt.h>

#include <textmode.cpp>
#include <kb.cpp>

TextMode tm;
KB       kb;

int main() {

    tm.start()->hide()->cls(0x17);
    tm.frame(1,1,30,4,1)->color(0x1E)->cursor(3,1)->printutf8(" Простой таймер ");

    dword start  = TIMERD / 1000;
    dword pvalue = 0;
    
    for(;;) {

        if (kb.hit()) { start = TIMERD / 1000; pvalue = start - 1; }

        dword current = TIMERD / 1000;

        if (pvalue != current) {
            
            pvalue = current;
            dword df = current - start;
            word  sec = df % 60;
            word  min = df / 60;
             
            tm.cursor(3, 3)->color(0x1F);

            if (min < 10) tm.printch('0');
            tm.printint(min); tm.print(sec < 10 ? ":0" : ":");
            tm.printint(sec); 
        }
    }
}
