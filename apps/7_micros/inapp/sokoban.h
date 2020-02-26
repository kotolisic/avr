
enum SokobanSprite {

    SOKOBAN_BRICK   = 1,
    SOKOBAN_BOX     = 2,
    SOKOBAN_PLACE   = 3,
    SOKOBAN_WOOD    = 4,
    SOKOBAN_PLAYER  = 5,
};

// Уровень сокобана
static const byte sokoban_level[1][64] =
{
    {   // Уровень 1
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x55, 0x40, 0x00, 0x00,
        0x00, 0x40, 0x40, 0x00, 0x00,
        0x00, 0x60, 0x40, 0x00, 0x00,
        0x05, 0x42, 0x50, 0x00, 0x00,
        0x04, 0x22, 0x10, 0x00, 0x00,
        0x54, 0x45, 0x10, 0x15, 0x54,
        0x40, 0x45, 0x15, 0x50, 0xf4,
        0x48, 0x20, 0x00, 0x00, 0xf4,

        0x55, 0x45, 0x44, 0x50, 0xf4,
        //0x55, 0x45, 0x44, 0x52, 0xf4,

        0x00, 0x40, 0x05, 0x55, 0x54,
        0x00, 0x55, 0x54, 0x00, 0x00,
        // Положение игрока
        11, 9,
        0x00, 0x00
    }
};
