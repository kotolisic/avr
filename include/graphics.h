
int g_cursor_x, region_x1, region_y1,
    g_cursor_y, region_x2, region_y2,
    g_cursor_cl,
    mouse_x,
    mouse_y;
    
char enamouse;    

// Сохранить фон мыши 21 x 12 
char mouse_bk[14][8];

static const unsigned int mouse_icon[14] = {

    0b1100000000000000,
    0b1111000000000000,
    0b1110110000000000,
    0b1110101100000000,
    0b1110101011000000,
    0b1110101010110000,
    0b1110101010101100,
    0b1110101010111111,
    0b1110111110110000,
    0b1111001110101100,
    0b1100000011101100,
    0b0000000011101011,
    0b0000000000111011,
    0b0000000000001100
};
