#include <avrio.c>
#include <textmode.c>

// id: 0-left, 1-right; active = 0/1
void panel(char id, char active, char letter) {

    color(0x1B);

    int i;
    int x1 = id ? 40 : 0;
    int x2 = id ? 79 : 39;

    block(x1, 0, x2, 22, 0x1B);
    frame(x1,  0, x2, 22, 1);

    // Наименования панелей
    color(active ? 0x30 : 0x1B);
    locate(x1 + 17, 0); print(" X:\\ "); printc(x1 + 18, 0, letter);
    locate(x1 + 5, 1); color(0x1E); print("Name        Size     Date    Time");

    color(0x1B);

    // Вертикальные полосы
    for (i = 1; i < 20; i++) {
        printc(x1 + 13, i, 0xB3);
        printc(x1 + 23, i, 0xB3);
        printc(x1 + 32, i, 0xB3);
    }
    for (i = 1; i < 39; i++) printc(x1 + i, 20, 0xC4);

    printc(x1 + 13, 0, 0xD1); printc(x1 + 13, 20, 0xC1);
    printc(x1 + 23, 0, 0xD1); printc(x1 + 23, 20, 0xC1);
    printc(x1 + 32, 0, 0xD1); printc(x1 + 32, 20, 0xC1);
    printc(x1,     20, 0xC7); printc(x1 + 39, 20, 0xB6);
}

// Форматированная печать
void print_format(char* s) {

    int i = 0;
    while (s[i]) {

        switch (s[i]) {
            case 1: color(0x07); break;
            case 2: color(0x30); break;
            default: printch(s[i]); break;
        }
        i++;
    }
}

// Перерисовать полностью экран
void redraw() {

    cls(0x07);

    // Две панели
    panel(0, 1, 'C');
    panel(1, 0, 'A');

    // Строки с подписями
    locate(0, 24);
    print_format("\x01""1""\x02""Help  ""\x01"" 2""\x02""Menu  ""\x01"" 3""\x02""View  ""\x01"" 4""\x02""Edit  ""\x01"" 5""\x02""Copy  ""\x01"" 6""\x02""RenMov""\x01"" 7""\x02""MkDir ""\x01"" 8""\x02""Delete""\x01"" 9""\x02""PullDn""\x01"" 10""\x02""Quit ");

    // Приглашение командной строки
    cursor(0, 23);
    color(0x07);
    print("C:>");
}

#define defmem(R, B) unsigned char* R = (unsigned char*) B

int main() {

    redraw();
}
