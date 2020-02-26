#include "sokoban.h"

// ---------------------------------------------------------------------
class AppSokoban {

protected:

    int  player_x, player_y;
    byte freecnt;
    int  steps;

public:

    void main() {

        g.start();

        byte current_level = 0;
        byte lvl[240];
        byte ch, ob;
        char movex, movey, moved;

        load_level(current_level, lvl);
        draw_level(lvl);

        for (;;) {

            movex = 0;
            movey = 0;
            moved = 0;

            // Нарисовать игрока
            show_stat(lvl);
            sprite(player_x, player_y, SOKOBAN_PLAYER);

            // Победа
            if (freecnt == 0) { you_win(); break; }

            ch = kb.getch();
            update_player(lvl);

            if      (ch == VK_UP)    { movex =  0; movey = -1; }
            else if (ch == VK_DOWN)  { movex =  0; movey =  1; }
            else if (ch == VK_LEFT)  { movex = -1; movey =  0; }
            else if (ch == VK_RIGHT) { movex =  1; movey =  0; }

            // Зарегистрировано перещение
            if (movex || movey) {

                // Проверка, куда перемещается
                ob = get_obj_hi(player_x + movex, player_y + movey, lvl);

                if (ob == SOKOBAN_BRICK) {
                    // Загораживает стена
                }
                else if (ob == SOKOBAN_BOX) {

                    ob = get_obj_hi(player_x + 2*movex, player_y + 2*movey, lvl);

                    if (ob == SOKOBAN_BOX || ob == SOKOBAN_BRICK) {
                        // За ящиком стена или ящик
                    }
                    else {

                        moved = 1;

                        // Очистить коробку на прежнем месте и передвинуть
                        lvl[20*(player_y + 1*movey) + (player_x + 1*movex)] &= 0xF0;
                        lvl[20*(player_y + 2*movey) + (player_x + 2*movex)] |= SOKOBAN_BOX;

                        update_sprite(player_x + 1*movex, player_y + 1*movey, lvl);
                        update_sprite(player_x + 2*movex, player_y + 2*movey, lvl);
                    }
                }
                else moved = 1;
            }

            // Перемещение доступно
            if (moved) {

                player_x += movex;
                player_y += movey;
                steps++;
            }
        }
    }

    void you_win() {

        for (int i = 0; i < 200; i++)
        for (int j = i&1; j < 320; j += 2)
            g.pset(j, i, 0);


        g.wiped(0)->window(50, 50, 200, 40, "Вы прошли уровень");
        g.cursor(54, 70)->color(0)->print("Перейти к следующему...");
        kb.getch();
    }

    // Кол-во остатков
    byte count_free(byte lvl[]) {

        byte cnt = 0;

        for (int i = 0; i < 12; i++)
        for (int j = 0; j < 20; j++) {

            byte hi = lvl[20*i + j] >> 4;
            byte lo = lvl[20*i + j] & 0x0F;

            if (hi == SOKOBAN_PLACE && lo != SOKOBAN_BOX)
                cnt++;
        }

        return cnt;
    }

    // Показать статистику
    void show_stat(byte lvl[]) {

        freecnt = count_free(lvl);

        g.cursor(0, 0)->color(15)->wiped(0x10);
        g.print("Ящиков: ");
        g.printint(freecnt);
        g.print("  Ход: ");
        g.printint(steps);
        g.print("   ");
    }

    // Объект, который стоит на уровне игрока
    byte get_obj_hi(int x, int y, byte lvl[]) {
        return lvl[20*y + x] & 0x0F;
    }

    // Обновить спрайт
    void update_sprite(int x, int y, byte lvl[]) {

        byte hi = lvl[20*y + x] >> 4;
        byte lo = lvl[20*y + x] & 0x0F;

        // Гарисовать Place, если нет ничего (например ящика)
        if (lo == 0 && hi == SOKOBAN_PLACE)
            sprite(x, y, hi);
        else
            sprite(x, y, lo);
    }

    // Чтобы поменьше писать
    void update_player(byte lvl[]) { update_sprite(player_x, player_y, lvl); }

    // Загрузка уровня в память
    void load_level(int level_id, byte store[]) {

        steps = 0;

        for (int i = 0; i < 12; i++) {

            for (int j = 0; j < 20; j++) {

                byte vb = sokoban_level[level_id][ (j>>2) + (i*5) ];
                byte sp = (vb >> (6 - 2*(j & 3))) & 3;

                store[20*i + j] = (sp == SOKOBAN_PLACE) ? (SOKOBAN_PLACE << 4) : sp;
            }
        }

        // Записать позицию игрока
        player_x = sokoban_level[level_id][60];
        player_y = sokoban_level[level_id][61];
    }

    // Нарисовать уровень и загрузить его в память
    void draw_level(byte level[]) { // byte lvl

        g.cls(0);
        for (int i = 0; i < 12; i++)
        for (int j = 0; j < 20; j++) {
            update_sprite(j, i, level);
        }
    }

    // Нарисовать некоторый спрайт
    void sprite(int x, int y, int id) {

        char i, j, k = 0;

        x *= 16;
        y *= 16;

        switch (id) {

            // Пустота
            case 0:

                g.block(x, y, x+15, y+15, 0);
                break;

            // Кирпичи
            case SOKOBAN_BRICK:

                g.block(x, y, x+15, y+15, 3);
                for (i = 0; i < 16; i += 4) {
                    g.line(x,y+i,x+15,y+i,0);
                    for (j = k&1?3:7; j < 16; j += 8)
                        g.line(x+j,y+i,x+j,y+i+3,0);
                    k++;
                }
                break;

            // Доски
            case SOKOBAN_WOOD:
            case SOKOBAN_PLACE:

                g.block(x, y, x+15, y+15, 4);
                for (i = 0; i < 16; i += 4) {
                    g.line(x,y+i,x+15,y+i,0);
                }

                // 2 доски
                g.line(x+15,y,x+15,y+3,0);
                g.line(x+15,y+8,x+15,y+11,0);

                // 3 гвоздя
                g.pset(x+2,y+2,0);
                g.pset(x+2,y+10,0);
                g.pset(x+12,y+6,0);
                g.pset(x+12,y+14,0);

                // Это место для парковки
                if (id == SOKOBAN_PLACE) g.circle(x+8,y+8,3,7);

                break;

            // Коробка для сокобана
            case SOKOBAN_BOX:

                g.block(x, y, x+15, y+15, 6);
                g.lineb(x, y, x+15, y+3, 0);
                g.lineb(x, y+3, x+3, y+12, 0);
                g.lineb(x+12, y+3, x+15, y+12, 0);
                g.lineb(x, y+12, x+15, y+15, 0);
                g.line (x+3, y+6, x+12, y+6, 0);
                g.line (x+3, y+9, x+12, y+9, 0);
                break;

            case SOKOBAN_PLAYER:

                g.circle(x + 8, y + 3, 2, 7);
                g.line(x + 8, y + 6, x + 8, y + 10, 7);
                g.line(x + 8, y + 10, x + 5, y + 15, 7);
                g.line(x + 8, y + 10, x + 11, y + 15, 7);
                g.line(x + 8, y + 5, x + 4, y + 9, 7);
                g.line(x + 8, y + 5, x + 12, y + 9, 7);

                break;
        }
    }
};
