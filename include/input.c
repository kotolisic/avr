#include <textmode.c>

// Ввод строки в терминальном режиме (через Enter)
void input(char* buffer, int max) {

    unsigned char k;
    int sx = teletype_x,
        sy = teletype_y,
        at = 0, i, ln = 0;

    buffer[0] = 0;

    for (;;) {

        k = getch();

        // Подтвердить ввод (Enter)
        if (k == 10) { printch(10); break; }

        // Влево
        if (k == 4) {
            at--; if (at < 0) at = 0;
        }
        // Вправо
        else if (k == 2) {
            at++; if (at > ln) at = ln;
        }
        // Удалить предыдущий символ
        else if (k == 8) {

            if (at > 0) {

                // Сдвинуть всю строку
                for (i = at - 1; i < ln; i++) buffer[i] = buffer[i+1];

                // Предпоследний символ удалить. Последний сделать 0
                buffer[ln-1] = ' ';  buffer[ln] = 0;

                // Сдвиг влево
                at--; ln--;
            }
        }
        // Del
        else if (k == 24) {

            if (at < ln) {

                for (i = at; i < ln; i++) buffer[i] = buffer[i+1];
                buffer[ln-1] = ' ';  buffer[ln] = 0; ln--;
            }
        }
        // Home, End
        else if (k == 5) { at = 0; }
        else if (k == 6) { at = ln; }        

        // else if (k < 32) { continue; }
        
        // Есть лимит на вставку символов
        else if (ln < max) {

            // Вставить новый символ в текущее положение
            for (i = ln; i > at; i--) buffer[i] = buffer[i-1];

            buffer[at++] = k;
            buffer[++ln] = 0;
        }

        // Скрыть курсор на время
        locate(sx, sy); print(buffer);

        // Переустановка курсора на нужное место
        cursor((sx + at) % 80, (sx + at) / 80 + sy);
    }
}

