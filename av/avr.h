#include "SDL.h"
#include "ansi3.h"
#include "ansi16.h"
#include <stdlib.h>
#include <stdio.h>

static const char* ds_brcs[4][8] = {
    {"cc", "ne", "pl", "vc", "ge", "hc", "tc", "id"},
    {"cs", "eq", "mi", "vs", "lt", "hs", "ts", "ie"},
    {"sec","sez","sen","sev","ses","seh","set","sei"},
    {"clc","clz","cln","clv","cls","clh","clt","cli"}
};

struct CPUFlags {
    int c, z, n, v, s, h, t, i;
};

enum AVROpcodes {

    UNDEFINED = 0,
    CPC,    SBC,    ADD,    CP,     SUB,    ADC,    AND,    EOR,    OR,
    CPI,    SBCI,   SUBI,   ORI,    ANDI,   ADIW,   SBIW,
    RJMP,   RCALL,  RET,    RETI,   BRBS,   BRBC,
    SBRC,   SBRS,   SBIC,   SBIS,   CPSE,
    ICALL,  EICALL, IJMP,   EIJMP,  CALL,   JMP,
    LDI,    MOV,    LDS,    STS,    MOVW,   BLD,    BST,    XCH,
    INC,    DEC,    LSR,    ASR,    ROR,    NEG,    COM,    SWAP,   LAC,
    LAS,    LAT,    BCLR,   BSET,   CBI,    SBI,
    LDX,    LDX_,   LD_X,   LDY_,   LD_Y,   LDYQ,   LDZ_,   LD_Z,   LDZQ,
    STX,    STX_,   ST_X,   STY_,   ST_Y,   STYQ,   STZ_,   ST_Z,   STZQ,
    LPM0Z,  LPMRZ,  LPMRZ_, ELPM0Z, ELPMRZ, ELPMRZ_, SPM,   SPM2,
    SLEEP,  WDR,    BREAK,  NOP,    IN,     OUT,    PUSH,   POP,    DES
};

// Палитра для DOS 320x200 MODE 13h
static const int DOS_13[256] =
{
  0x000000, 0x000088, 0x008800, 0x008888, 0x880000, 0x880088, 0x885500, 0xaaaaaa, // 0
  0x555555, 0x5555ff, 0x55ff55, 0x55ffff, 0xff5555, 0xff55ff, 0xffff55, 0xffffff, // 8
  0x000000, 0x141414, 0x202020, 0x2c2c2c, 0x383838, 0x454545, 0x515151, 0x616161, // 10
  0x717171, 0x828282, 0x929292, 0xa2a2a2, 0xb6b6b6, 0xcbcbcb, 0xe3e3e3, 0xffffff, // 18
  0x0000ff, 0x4100ff, 0x7d00ff, 0xbe00ff, 0xff00ff, 0xff00be, 0xff007d, 0xff0041, // 20
  0xff0000, 0xff4100, 0xff7d00, 0xffbe00, 0xffff00, 0xbeff00, 0x7dff00, 0x41ff00, // 28
  0x00ff00, 0x00ff41, 0x00ff7d, 0x00ffbe, 0x00ffff, 0x00beff, 0x007dff, 0x0041ff, // 30
  0x7d7dff, 0x9e7dff, 0xbe7dff, 0xdf7dff, 0xff7dff, 0xff7ddf, 0xff7dbe, 0xff7d9e, // 38
  0xff7d7d, 0xff9e7d, 0xffbe7d, 0xffdf7d, 0xffff7d, 0xdfff7d, 0xbeff7d, 0x9eff7d, // 40
  0x7dff7d, 0x7dff9e, 0x7dffbe, 0x7dffdf, 0x7dffff, 0x7ddfff, 0x7dbeff, 0x7d9eff, // 48
  0xb6b6ff, 0xc7b6ff, 0xdbb6ff, 0xebb6ff, 0xffb6ff, 0xffb6eb, 0xffb6db, 0xffb6c7, // 50
  0xffb6b6, 0xffc7b6, 0xffdbb6, 0xffebb6, 0xffffb6, 0xebffb6, 0xdbffb6, 0xc7ffb6, // 58
  0xb6ffb6, 0xb6ffc7, 0xb6ffdb, 0xb6ffeb, 0xb6ffff, 0xb6ebff, 0xb6dbff, 0xb6c7ff, // 60
  0x000071, 0x1c0071, 0x380071, 0x550071, 0x710071, 0x710055, 0x710038, 0x71001c, // 68
  0x710000, 0x711c00, 0x713800, 0x715500, 0x717100, 0x557100, 0x387100, 0x1c7100, // 70
  0x007100, 0x00711c, 0x007138, 0x007155, 0x007171, 0x005571, 0x003871, 0x001c71, // 78
  0x383871, 0x453871, 0x553871, 0x613871, 0x713871, 0x713861, 0x713855, 0x713845, // 80
  0x713838, 0x714538, 0x715538, 0x716138, 0x717138, 0x617138, 0x557138, 0x457138, // 88
  0x387138, 0x387145, 0x387155, 0x387161, 0x387171, 0x386171, 0x385571, 0x384571, // 90
  0x515171, 0x595171, 0x615171, 0x695171, 0x715171, 0x715169, 0x715161, 0x715159, // 98
  0x715151, 0x715951, 0x716151, 0x716951, 0x717151, 0x697151, 0x617151, 0x597151, // A0
  0x517151, 0x517159, 0x517161, 0x517169, 0x517171, 0x516971, 0x516171, 0x515971, // A8
  0x000041, 0x100041, 0x200041, 0x300041, 0x410041, 0x410030, 0x410020, 0x410010, // B0
  0x410000, 0x411000, 0x412000, 0x413000, 0x414100, 0x304100, 0x204100, 0x104100, // B8
  0x004100, 0x004110, 0x004120, 0x004130, 0x004141, 0x003041, 0x002041, 0x001041, // C0
  0x202041, 0x282041, 0x302041, 0x382041, 0x412041, 0x412038, 0x412030, 0x412028, // C8
  0x412020, 0x412820, 0x413020, 0x413820, 0x414120, 0x384120, 0x304120, 0x284120, // D0
  0x204120, 0x204128, 0x204130, 0x204138, 0x204141, 0x203841, 0x203041, 0x202841, // D8
  0x2c2c41, 0x302c41, 0x342c41, 0x3c2c41, 0x412c41, 0x412c3c, 0x412c34, 0x412c30, // E0
  0x412c2c, 0x41302c, 0x41342c, 0x413c2c, 0x41412c, 0x3c412c, 0x34412c, 0x30412c, // E8
  0x2c412c, 0x2c4130, 0x2c4134, 0x2c413c, 0x2c4141, 0x2c3c41, 0x2c3441, 0x2c3041, // F0
  0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000  // F8
};

// ---------------------------------------------------------------------

class APP {

protected:

    int width, height;

    SDL_Event    event;
    SDL_Surface* sdl_screen;

    // Дизассемблер
    // ---------------------------

    char    ds_line[128];           // Дизассемблированная строка
    int     ds_addresses[64];
    int     ds_start;               // Стартовый адрес дизассемблера
    int     ds_current;             // Текущий адрес
    int     ds_cursor;              // Положение курсора
    int     ds_debugger;            // Если =1, то находится в отладчике
    int     ds_tab;                 // Где находимся
    int     ds_brk[128];            // Брейкпоинты
    int     ds_brk_cnt;
    int     ds_dump_cursor;
    int     ds_dump_start;

    // Процессор
    // ---------------------------

    unsigned char   program[128*1024];  // Память программы
    unsigned char   sram   [1024*1024]; // Общая память, также включает регистры с портами (1 Mb)
    unsigned char   pvsram [96];        // Для отладчика
    int             map[65536];
    unsigned short  opcode, command;

    int  pc;
    int  cpu_halt;
    int  require_halt;
    uint instr_counter;
    int  cycles;
    int  count_per_frame;
    int  framecycle;

    // I/O
    int  membank;
    int  videomode;      // 0 - TEXT 80x25, 1 - 320x200, 2 - 256x240
    int  port_keyb_hit;
    int  port_keyb_xt;
    int  text_px, text_py;
    int  cursor_x, cursor_y, flash, flash_id, require_disp_update;
    int  mouse_x, mouse_y, mouse_cmd;
    int  timer;

    // Информация о SPI
    unsigned char spi_data;
    unsigned char spi_st;
    unsigned char spi_status;
    unsigned char spi_command;
    unsigned long spi_arg;
    unsigned long spi_lba;
    unsigned char spi_crc;
    unsigned char spi_resp;
    unsigned int  spi_phase;
    FILE*    spi_file;
    unsigned char spi_sector[512];

    struct CPUFlags flag;

public:

    APP(int, int, const char*);

    // Приложение
    void infinite();
    void loadfile(const char* fn);
    int  get_key(SDL_Event event);
    void update_screen();
    void display_update();
    void update_text_xy(int x, int y);
    void update_byte_scr(int addr);
    void swi_brk();
    int  get_mouse_x();
    int  get_mouse_y();

    // Рисование на экране
    void pset(int x, int y, uint color);
    void flip();
    void cls(uint);
    void print16char(int col, int row, unsigned char ch, uint cl);
    void print3char(int col, int row, unsigned char ch, uint cl);
    void print3(int col, int row, const char* s, uint cl);
    void print(int col, int row, const char* s, uint cl);

    // Дизассемблер
    int  ds_fetch(uint& addr);
    int  ds_info(uint addr);
    void ds_update();

    // Процессор
    // --------------------

    // Вспомогательные функции
    void assign();
    void config();
    void assign_mask(const char* mask, int opcode);
    int  fetch() { int data = program[pc] + program[pc+1]*256; pc = (pc + 2) & 0xffff; return data; }

    // Установка флагов
    unsigned int neg(unsigned int);
    void set_logic_flags(unsigned char);
    void set_subcarry_flag(int, int, int, int);
    void set_subtract_flag(int, int, int);
    void set_add_flag(int, int, int, int);
    void set_adiw_flag(int, int);
    void set_sbiw_flag(int, int);
    void set_lsr_flag(int, int);

    // Извлечение операндов
    int get_rd_index() { return (opcode & 0x1F0) >> 4; }
    int get_rr_index() { return (opcode & 0x00F) | ((opcode & 0x200)>>5); }
    int get_rd() { return sram[ get_rd_index() ]; }
    int get_rr() { return sram[ get_rr_index() ]; }
    int get_rdi() { return sram[ get_rd_index() | 0x10 ]; }
    int get_rri() { return sram[ get_rr_index() | 0x10 ]; }
    int get_imm8() { return (opcode & 0xF) + ((opcode & 0xF00) >> 4); }
    int get_ap() { return (opcode & 0x00F) | ((opcode & 0x600) >> 5); }
    int get_ka() { return (opcode & 0x00F) | ((opcode & 0x0C0) >> 2); }
    int get_qi() { return (opcode & 0x007) | ((opcode & 0xC00) >> 7) | ((opcode & 0x2000) >> 8); }
    int get_s3() { return (opcode & 0x070) >> 4; }
    int get_jmp() { return (opcode & 1) | ((opcode & 0x1F0) >> 3); }

    // 16 bit
    int get_S() { return sram[0x5D] + sram[0x5E]*256; }
    int get_X() { return sram[0x1A] + sram[0x1B]*256; }
    int get_Y() { return sram[0x1C] + sram[0x1D]*256; }
    int get_Z() { return sram[0x1E] + sram[0x1F]*256; }

    void put_S(unsigned short a) { sram[0x5D] = a & 0xFF; sram[0x5E] = (a >> 8) & 0xFF; }
    void put_X(unsigned short a) { sram[0x1A] = a & 0xFF; sram[0x1B] = (a >> 8) & 0xFF; }
    void put_Y(unsigned short a) { sram[0x1C] = a & 0xFF; sram[0x1D] = (a >> 8) & 0xFF; }
    void put_Z(unsigned short a) { sram[0x1E] = a & 0xFF; sram[0x1F] = (a >> 8) & 0xFF; }

    void put16(int a, unsigned short v) { sram[a] = v & 0xff; sram[a+1] = (v >> 8) & 0xff; }
    unsigned short get16(int a)  { return sram[a] + 256*sram[a+1]; }

    void put(int, unsigned char);
    void put_rd(unsigned char value) { sram[get_rd_index()] = value & 0xff; }
    void put_rr(unsigned char value) { sram[get_rr_index()] = value & 0xff; }
    void put_rdi(unsigned char value) { sram[get_rd_index() | 0x10] = value & 0xff; }
    unsigned char get(int);

    // Работа со стеком
    void push8(unsigned char v8) { unsigned short sp = get_S(); put(sp, v8); put_S((sp - 1) & 0xffff); }
    unsigned char pop8() { unsigned short sp = (get_S() + 1) & 0xffff; put_S(sp); return get(sp); }

    void push16(unsigned short v16) { push8(v16 & 0xff); push8(v16 >> 8); }
    unsigned short pop16() { int h = pop8(); int l = pop8(); return h*256 + l; }

    void byte_to_flag(unsigned char);
    unsigned char flag_to_byte();

    // Относительные переходы
    int skip_instr();
    int get_rjmp()   { return (pc + 2*((opcode & 0x800) > 0 ? ((opcode & 0x7FF) - 0x800) : (opcode & 0x7FF))) & 0x1FFFF; }
    int get_branch() { return (pc + 2*((opcode & 0x200) > 0 ? ((opcode & 0x1F8)>>3) - 0x40 : ((opcode & 0x1F8)>>3) )) & 0x1FFFF; }
    int step();

    // SPI
    unsigned char spi_read_data();
    unsigned char spi_read_status();
    void spi_write_data(unsigned char data);
    void spi_write_cmd(unsigned char data);

};
