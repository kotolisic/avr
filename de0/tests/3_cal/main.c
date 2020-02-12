#include <avrio.c>
#include <kbd.c>
#include <textmode.c>

int  leap;
long year;

// Список месяцев
static char* month[12] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

static char* days[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
static char  daysin[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// (x,y)-position, m - 0..11
int draw_month(int x, int y, int m, int st) {

    int i = 0, d = 1, max;

    // Дней в месяце
    max = daysin[m] + (m == 1 ? leap : 0);

    for (i = 0; i < max; i++) {

        color(st < 5 ? 0x07 : 0x0C);

        // Праздничные дни
        if (year == 2020) {

            switch (m) {

                case 0: if (d < 9) color(0x0C); break;
                case 1: if (d == 24) color(0x0C); break;
                case 2: if (d == 9) color(0x0C); break;
                case 4: if (d == 1 || d == 4 || d == 5 || d == 11) color(0x0C); break;
                case 5: if (d == 12) color(0x0C); break;
                case 10: if (d == 4) color(0x0C); break;
            }
        }

        locate(x + (d < 10 ? 1 : 0), y + st);

        printint(d);
        d++;
        st++;
        if (st == 7) { st = 0; x += 3; }
    }

    return st;
}

int main() {

    cls(0x07);
    year = 2020;

    int  i, k;
    long start = ((1461 * (year - 1)) >> 2) + 6;
    int  dd = start % 7;

    leap  = ((year & 3) == 0) ? 1 : 0;

    // Дни недели
    for (k = 0; k < 3; k++)
    for (i = 0; i < 7; i++) {

        teletype_cl = i < 5 ? 0x03 : 0x0B; // 0x0B
        locate(2, 1 + i + k*8);
        print(days[i]);
    }

    for (k = 0; k < 3; k++)
    for (i = 0; i < 4; i++) {

        int m = 4*k + i;
        teletype_cl = 0x0A;
        locate(12 + i*18, k*8); print(month[m]);
        dd = draw_month(5 + i*18, 1 + k*8, m, dd);
    }

    locate(75, 24); printint(year);
}
