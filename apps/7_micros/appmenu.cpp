#define APPMENU_MAX     7

enum MenuItems {

    MENU_CALENDAR   = 3,
    MENU_SOKOBAN    = 6

};

static const char* appmenu_list[] = {

    /* 0 */ "Нортон",
    /* 1 */ "Калькулятор",
    /* 2 */ "Секундомер",
    /* 3 */ "Календарь",
    /* 4 */ "Simple BASIC",
    /* 5 */ "Трехмерность",
    /* 6 */ "Сокобан",
    /* 7 */ "Лабиринт"
};

class AppMenu {
protected:

    int selected;


public:

    // Нарисовать бейдж
    void badge(int s) {

        // Специальная обводка
        g.line(220, 3,  220 + 16*6, 3, 0);
        g.line(220, 20, 220 + 16*6, 20, 0);

        // Фирменный значок
        for (char i = 0; i < 16; i++) {
            for (int j = 0; j < 128; j++) {

                int x = 220 + j;
                if (x > 220 && x < 220 + 16*6) {
                    g.pset(x, 4 + i, ((s + j + i) >> 4));
                }
            }
        }
    }

    // Рисуется один элемент меню
    void draw_menu_element(byte id, byte cl) {

        g.cursor(8, 32 + 16*id);
        g.color(cl);
        g.print(appmenu_list[id]);
    }

    // Основное меню
    int selector() {

        byte keyb, cursor = 0;
        word badge_offset = 0;
        word ptimer = 0;

        selected = 0;

        g.start();
        g.block(0,0,319,199,1);
        g.block(0,0,319,24,7);
        g.cursor(8, 4)->color(0)->print("Выбор приложения");

        badge(badge_offset);

        // Вывести все меню
        for (int i = 0; i < 8; i++) draw_menu_element(i, cursor == i ? 10 : 7);

        for (;;) {

            word ctimer = TIMERW;

            // Есть нажатие на клавишу
            if ((keyb = kb.inkey())) {

                // Кнопки управления
                if (keyb == VK_DOWN || keyb == VK_UP) {

                    draw_menu_element(selected, 7);
                    selected = (keyb == VK_DOWN ? selected + 1 : selected - 1);

                    // Перемотка
                    if (selected < 0)
                        selected = APPMENU_MAX;
                    else if (selected > APPMENU_MAX)
                        selected = 0;

                    draw_menu_element(selected, 10);
                }
                // Включить программу
                else if (keyb == VK_ENTER) {
                    break;
                }

            }

            // Оживить бейдж
            if (ctimer - ptimer > 100) {

                badge(badge_offset);
                badge_offset++;
                ptimer = ctimer;
            }
        }

        return selected;
    }
};
