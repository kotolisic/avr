#include "sokoban.h"

// ---------------------------------------------------------------------
class AppSokoban {

protected:

    int player_x, player_y;

public:

    void main() {

        g.start();
        byte lvl[240];

        load_level(0, lvl);
        draw_level(lvl);

        kb.getch();
    }

    // Загрузка уровня в память
    void load_level(int level_id, byte store[]) {

        for (int i = 0; i < 12; i++) {

            for (int j = 0; j < 20; j++) {

                byte vb = sokoban_level[level_id][ (j>>2) + (i*5) ];
                byte sp = (vb >> (6 - 2*(j & 3))) & 3;
                store[20*i + j] = sp;
            }
        }

        player_x = sokoban_level[level_id][60];
        player_y = sokoban_level[level_id][61];

        store[player_y*20 + player_x] = SOKOBAN_PLAYER;
    }

    // Нарисовать уровень и загрузить его в память
    void draw_level(byte level[]) { // byte lvl

        g.cls(0);
        for (int i = 0; i < 12; i++) 
        for (int j = 0; j < 20; j++)
            sprite(j, i, level[20*i + j]);        
    }

    // Нарисовать некоторый спрайт
    void sprite(int x, int y, int id) {

        char i, j, k = 0;

        x *= 16;
        y *= 16;

        switch (id) {

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

                g.block(x, y, x+15, y+15, 6);
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

                // Это место для установки
                if (id == SOKOBAN_PLACE)
                    g.circle(x+8,y+8,5,15);

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

                g.circle(x + 8, y + 3, 2, 15);
                g.line(x + 8, y + 6, x + 8, y + 10, 15);
                g.line(x + 8, y + 10, x + 5, y + 15, 15);
                g.line(x + 8, y + 10, x + 11, y + 15, 15);
                g.line(x + 8, y + 5, x + 4, y + 9, 15);
                g.line(x + 8, y + 5, x + 12, y + 9, 15);

                break;
        }
    }
};
