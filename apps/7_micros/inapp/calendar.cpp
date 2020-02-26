
// Список месяцев
static const char* cal_month[12] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

static const char* cal_days[7] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
static const char  cal_daysin[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class AppCalendar {
    
protected:

    int  leap;
    long year;

public:

    void main() {

        t.start()->hide()->cls(0x07);

        year = 2020;

        int  i, k;
        long start = ((1461 * (year - 1)) >> 2) + 6;
        int  dd = start % 7;

        leap  = ((year & 3) == 0) ? 1 : 0;

        // Дни недели
        for (k = 0; k < 3; k++)
        for (i = 0; i < 7; i++) {

            t.color(i < 5 ? 0x03 : 0x0B);
            t.cursor(2, 1 + i + k*8);
            t.print(cal_days[i]);
        }

        for (k = 0; k < 3; k++)
        for (i = 0; i < 4; i++) {

            int m = 4*k + i;
            
            t.color(0x0A);
            t.cursor(12 + i*18, k*8);
            t.print(cal_month[m]);

            dd = draw_month(5 + i*18, 1 + k*8, m, dd);
        }

        t.cursor(75, 24);
        t.printint(year);

        kb.getch();
    }
    
    // (x,y)-position, m - 0..11
    int draw_month(int x, int y, int m, int st) {

        int i = 0, d = 1, max;

        // Дней в месяце
        max = cal_daysin[m] + (m == 1 ? leap : 0);

        for (i = 0; i < max; i++) {

            t.color(st < 5 ? 0x07 : 0x0C);

            // Праздничные дни
            if (year == 2020) {

                switch (m) {

                    case 0: if (d < 9) t.color(0x0C); break;
                    case 1: if (d == 24) t.color(0x0C); break;
                    case 2: if (d == 9) t.color(0x0C); break;
                    case 4: if (d == 1 || d == 4 || d == 5 || d == 11) t.color(0x0C); break;
                    case 5: if (d == 12) t.color(0x0C); break;
                    case 10: if (d == 4) t.color(0x0C); break;
                }
            }

            t.cursor(x + (d < 10 ? 1 : 0), y + st);
            t.printint(d);

            d++;
            st++;
            if (st == 7) { st = 0; x += 3; }
        }

        return st;
    }

};
