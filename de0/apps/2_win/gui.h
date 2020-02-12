enum window_class_enum {

    WIN_ENABLED     = 1,        // Окно работает
    WIN_ACTIVE      = 2,        // Окно активно в данный момент
    WIN_NEED_REDRAW = 4,        // Требуется обновление
    WIN_VISIBLE     = 8         // Видимо на экране (не свернуто)
};

#define MAX_WINDOWS 16

struct window_class {

    int     hwnd;
    int     x, y;
    int     w, h;
    char    flags;
    char*   name;

};

// Максимальне кол-во окон в системе
struct window_class windows[ MAX_WINDOWS ];

// Последнее активное окно
int  hwnd_id;
char window_hwnd_active;
