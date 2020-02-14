#include "gui.h"

// Рисовать кнопку
void button(int x, int y, int w, int h, int pressed) {

    int x2 = x + w, y2 = y + h;

    block(x,  y, x2, y2, 7);

    line(x,  y, x2,  y, pressed ? 0  : 15);
    line(x,  y, x,  y2, pressed ? 0  : 15);
    line(x2, y, x2, y2, pressed ? 15 : 0);
    line(x, y2, x2, y2, pressed ? 15 : 0);

    if (pressed) {
        line(x+1, y+1, x+1, y2-1, 8);
        line(x+1, y+1, x2-1, y+1, 8);
    } else {
        line(x2-1, y+1,  x2-1, y2-1, 8);
        line(x+1,  y2-1, x2-1, y2-1, 8);
    }
}

// Кнопка со значком "крестик"
void button_close(int x, int y, char pressed) {

    // Значок крестика
    button(x, y, 9, 9, pressed);

    x += pressed;
    y += pressed;
    line(x + 6, y + 2, x + 2, y + 6, 0);
    line(x + 2, y + 2, x + 6, y + 6, 0);
}

// Свернуть окно
void button_min(int x, int y, char pressed) {

    button(x, y, 9, 9, pressed);

    x += pressed;
    y += pressed;
    line(x + 2, y + 6, x + 6, y + 6, 0);
}

// Нарисовать окно
void window(int x, int y, int w, int h, char* name, char flags) {

    int x2 = x + w,
        y2 = y + h;

    block(x, y, x2, y2, 7);
    line(x2, y, x2, y2, 0);
    line(x, y2, x2, y2, 0);
    line(x + 1, y + 1, x2 - 1, y  + 1, 15);
    line(x + 1, y + 1, x  + 1, y2 - 1, 15);
    line(x + 1, y2 - 1, x2 - 1, y2 - 1, 8);
    line(x2 - 1, y + 1, x2 - 1, y2 - 1, 8);

    block(x + 3, y + 3, x2 - 3, y + 13, flags & WIN_ACTIVE ? 1 : 8);
    cursor(x + 5, y + 5); color(15); print4(name);

    button_close(x2 - 12, y + 4, 0);
    button_min(x2 - 23, y + 4, 0);
}

// Рисовать можно везде
void gui_windows_region_all() {

    region_x1 = 0;
    region_y1 = 0;
    region_x2 = 319;
    region_y2 = 199;
}

// Панель управления задачами
void start_panel() {

    gui_windows_region_all();

    block(0, 184, 319, 199, 7);
    line(0, 185, 319, 185, 15);

    /*
    // Рисовать кнопку "Пуск", но она сейчас не нужна
    button(2, 187, 30, 11, 0);
    block(4, 189, 8, 195, 4);
    cursor(11, 189); color(0); print4("CTAPT");
    */
}

// Инициализация окон в системе
void gui_init() {
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].flags = 0;
    }
    
    hwnd_id = 0;
    window_hwnd_active = 0;
}

// Апи для отрисовки окна
void gui_window_paint(int hwnd) {

    // Ограничение максимально достижимой области
    region_x1 = 0;   region_y1 = 0;
    region_x2 = 319; region_y2 = 183;

    // И уже отрисовка окна
    window(windows[hwnd].x, windows[hwnd].y,
           windows[hwnd].w, windows[hwnd].h,
           windows[hwnd].name,
           windows[hwnd].flags);

    // Окно обновилось, снять флаг
    windows[hwnd].flags &= ~WIN_NEED_REDRAW;

    // Восстановить возможность рисовать везде
    gui_windows_region_all();
}

// Установка ограничителей для окна
void gui_window_region_area(int hwnd) {

    region_x1 = windows[hwnd].x + 3;
    region_y1 = windows[hwnd].y + 15;
    region_x2 = windows[hwnd].x + windows[hwnd].w - 3;
    region_y2 = windows[hwnd].y + windows[hwnd].h - 3;

    if (region_y2 > 183) region_y2 = 183;
}

// Рисование блока относительно внутренного окна
void gui_block(int hwnd, int x1, int y1, int x2, int y2, char cl) {

    gui_window_region_area(hwnd);

    int xw = windows[hwnd].x + 3;
    int yw = windows[hwnd].y + 15;

    block(x1 + xw, y1 + yw, x2 + xw, y2 + yw, cl);
    gui_windows_region_all();
}

// ---------------------------------------------------------------------
// Управление окнами
// ---------------------------------------------------------------------

// Не передается `title`, он потом рисуется отдельно
int create_window(int x, int y, int w, int h, const char* name) {

    for (int i = 0; i < MAX_WINDOWS; i++) {

        if (windows[i].flags & WIN_ENABLED) {
            windows[i].flags &= ~WIN_ACTIVE;
            continue;
        }

        hwnd_id++;

        // Есть свободное место для окна
        windows[i].hwnd = hwnd_id;
        windows[i].flags |= (WIN_ENABLED | WIN_ACTIVE | WIN_VISIBLE);
        windows[i].x = x;
        windows[i].y = y;
        windows[i].w = w + 6;
        windows[i].h = h + 18;
        windows[i].name = (char*)name;

        // Последнее активное окно
        window_hwnd_active = hwnd_id;

        // @todo update start panel

        return i;
    }

    return -1;
}


// Поиск окна по (X, Y)
int search_hwnd(int x, int y) {

    int hwnd = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {

        char flags = windows[i].flags;

        // Окно есть, и окно видимо
        if ((flags & WIN_ENABLED) && (flags & WIN_VISIBLE)) {

            int x1 = windows[i].x;
            int y1 = windows[i].y;
            int x2 = windows[i].x + windows[i].w;
            int y2 = windows[i].y + windows[i].h;

            if (x1 <= x && x <= x2 && y1 <= y && y <= y2)
                hwnd = windows[i].hwnd;
        }
    }

    return hwnd;
}
