#include <avrio.c>
#include <kbd.c>
#include <graphics.c>

// z(i+1) = z(i)^2 + C[r,i]
int bailout(float r, float i) {
    
    int k;
    float zi = 0.0, zr = 0.0;    
    for (k = 0; k < 16; k++) {
        
        float r_ = zr*zr - zi*zi + r;
        float i_ = 2*zi*zr + i;
        
        zr = r_; zi = i_;    
        
        if (zr*zr + zi*zi > 4.0) {
            return k;
        }                
    }
    
    return 0;
}

void mandelbrot(float ox, float oy, float mag) {

    for (int y = -100; y < 100; y++) {
    for (int x = -160; x < 160; x++) {

        float r = ((float)x * mag + ox),
              i = ((float)y * mag + oy);

        int b = bailout(r, i);
        pset(160 + x, 100 + y, b);
    } }
}

int main() {

    screen3();

    float mag = 1.0 / 64.0, ox = 0.0, oy = 0.0;

    for (;;) {
        
        mandelbrot(ox, oy, mag);

        char k = getch();

        // Управление
        if (k == 1) oy -= 25 * mag;
        if (k == 2) ox += 40 * mag;
        if (k == 3) oy += 25 * mag;
        if (k == 4) ox -= 40 * mag;

        if (k == 'w') mag /= 2.0;
        if (k == 's') mag *= 2.0;
    }
    
}
