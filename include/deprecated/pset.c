#ifndef PSETC_INC
#define PSETC_INC

// Рисование точки
void pset(unsigned char x, unsigned char y, unsigned char cl) {

    unsigned char* m = (unsigned char*)0xc000;

    int a = (y<<6) + (x>>2);
    int b = (3^(x&3))<<1;

    m[a] = (m[a] & ~(3<<b)) | ((cl&3)<<b);
}

#endif