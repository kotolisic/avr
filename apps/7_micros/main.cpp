// #include <avr/pgmspace.h>

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

#include "inapp/calendar.cpp"
#include "inapp/sokoban.cpp"
#include "appmenu.cpp"

AppMenu     appmenu;
AppCalendar calendar;
AppSokoban  sokoban;

int main() {

    for (;;) {

        byte program_id = appmenu.selector();

        switch (program_id) {

            case MENU_SOKOBAN: sokoban.main(); break;
            case MENU_CALENDAR: calendar.main(); break;

            default:

                g.window(40, 50, 240, 50, "Ошибка!");
                g.cursor(44, 75)->color(0)->print("Программа пока не реализована");
                kb.getch();
                break;
        }
    }
}
