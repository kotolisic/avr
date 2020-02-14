#include <avrio.c>
#include <kbd.c>
#include <graphics.c>
#include <line.c>
#include <print4.c>

#include "gui.c"

class test {

protected:
    int h1, h2;

public:

    void hello() {
        
        // Создать окно
        int h1 = create_window(10, 10, 128, 128, "Demoscene");
        int h2 = create_window(100, 30, 128, 128, "Tiny Windows");

        // -- Это уже само окно должно делать
        gui_window_paint(h1); // gui_block(hwnd, 0, 0, 256, 128, 0);
        gui_window_paint(h2); // gui_block(hwnd, 0, 0, 256, 128, 0);

    }

};

int main() {

    screen3(1);
    cls(3);

    gui_init();
    start_panel();

    test ab;
    ab.hello();

    char pmstat = 0, cmstat = 0;

    for (;;) {

        int mx = get_mouse_x();
        int my = get_mouse_y();
        char mb_dn = 0, mb_up = 0;

        // Сместить мышь
        if (mx != mouse_x || my != mouse_y) mouse_move(mx, my);

        // Отслеживание нажатия мыши
        // ---------------------------------
        cmstat = get_mouse_st();
        if      ((cmstat & 1) && (pmstat & 1) == 0) mb_dn = 1;
        else if ((cmstat & 1) == 0 && (pmstat & 1)) mb_up = 1;
        pmstat = cmstat;
        // ---------------------------------

        if (mb_dn) {

            int new_hwnd = search_hwnd(mx, my);

            // if (new_hwnd && new_hwnd != window_hwnd_active) { .. }

            block(0, 0, 319, 8, 3); cursor(0, 0); color(15); print4l(new_hwnd);

            // Если есть отличие от старого hwnd, то вытолкнуть наверх окно
        }
    }
}
