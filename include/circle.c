// Рисование окружности
void circle(int xc, int yc, int r, unsigned char c) {

    int x = 0;
    int y = r;
    int d = 3 - 2*y;

    while (x <= y) {

        // --
        pset(xc - x, yc + y, c);
        pset(xc + x, yc + y, c);
        pset(xc - x, yc - y, c);
        pset(xc + x, yc - y, c);
        pset(xc + y, yc + x, c);
        pset(xc - y, yc + x, c);
        pset(xc + y, yc - x, c);
        pset(xc - y, yc - x, c);
        // ...

        d += 4*x + 6;
        if (d >= 0) {
            d += 4*(1 - y);
            y--;
        }

        x++;
    }
}