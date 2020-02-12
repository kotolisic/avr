// #define fastpset false

// Рисование линии
void line(int x1, int y1, int x2, int y2, char cl) {

    if (y2 < y1) {
        x1 ^= x2; x2 ^= x1; x1 ^= x2;
        y1 ^= y2; y2 ^= y1; y1 ^= y2; 
    }

    int deltax = x2 > x1 ? x2 - x1 : x1 - x2;
    int deltay = y2 - y1;
    int signx  = x1 < x2 ? 1 : -1;

    int error2;
    int error = deltax - deltay;

    while (x1 != x2 || y1 != y2)
    {
        pset(x1, y1, cl);
        error2 = error * 2;

        if (error2 > -deltay) {
            error -= deltay;
            x1 += signx;
        }

        if (error2 < deltax) {
            error += deltax;
            y1 += 1;
        }
    }

    pset(x1, y1, cl);
};

void lineb(int x1, int y1, int x2, int y2, char cl) {

    line(x1, y1, x2, y1, cl);
    line(x2, y1, x2, y2, cl);
    line(x2, y2, x1, y2, cl);
    line(x1, y2, x1, y1, cl);
}
