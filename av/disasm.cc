#include "avr.h"

// Прочесть следующий опкод
int APP::ds_fetch(uint& addr) {

    int L = program[addr];
    addr = (addr + 1) & (128*1024 - 1);

    int H = program[addr];
    addr = (addr + 1) & (128*1024 - 1);

    return H*256 + L;
}

// Дизассемблировать адрес
int APP::ds_info(uint addr) {

    int pvaddr = addr;

    // Прочесть опкод
    int opcode = ds_fetch(addr);

    // Непосредственное значение, если нужно
    int K = ((opcode & 0xF00) >> 4) | (opcode & 0xF);

    // Номера регистров и портов
    int Rd = (opcode & 0x1F0) >> 4;
    int Rr = (opcode & 0x00F) | ((opcode & 0x200) >> 5);
    int Ap = (opcode & 0x00F) | ((opcode & 0x600) >> 5);
    int Ka = (opcode & 0x00F) | ((opcode & 0x0C0) >> 2);

    // ADDR[7:0] = (~INST[8], INST[8], INST[10], INST[9], INST[3], INST[2], INST[1], INST[0])
    int Ld = (opcode & 0x00F) | ((opcode & 0x600) >> 5) | ((opcode & 0x100) >> 2) | (((~opcode) & 0x100) >> 1);

    // 00q0 qq00 0000 0qqq
    int Qi = (opcode & 0x007) | ((opcode & 0xC00)>>7) | ((opcode & 0x2000) >> 8);

    // Относительный переход
    int Rjmp = addr + 2*((opcode & 0x800) > 0 ? (opcode & 0x7FF) - 0x800 : (opcode & 0x7FF));
    int Bjmp = addr + 2*((opcode & 0x200) > 0 ? ((opcode & 0x1F8)>>3) - 0x40 : ((opcode & 0x1F8)>>3) );
    int Bit7 = opcode & 7;
    int bit7s = (opcode & 0x70) >> 4;
    int jmpfar = (((opcode & 0x1F0) >> 3) | (opcode & 1)) << 16;
    int append;

    // Получение всевозможных мнемоник
    char name_Rd[32];   sprintf(name_Rd,    "r%d", Rd);
    char name_Rr[32];   sprintf(name_Rr,    "r%d", Rr);
    char name_Rdi[32];  sprintf(name_Rdi,   "r%d", (0x10 | Rd));
    char name_K[32];    sprintf(name_K,     "$%02X", K);
    char name_Ap[32];   sprintf(name_Ap,    "$%02X", Ap);
    char name_Ap8[32];  sprintf(name_Ap8,   "$%02X", (opcode & 0xF8) >> 3);
    char name_rjmp[32]; sprintf(name_rjmp,  "$%04X", Rjmp);
    char name_bjmp[32]; sprintf(name_bjmp,  "$%04X", Bjmp);
    char name_lds[32];  sprintf(name_lds,   "$%02X", Ld);
    char name_Rd4[32];  sprintf(name_Rd4,   "r%d", (Rd & 0xF)*2);
    char name_Rr4[32];  sprintf(name_Rr4,   "r%d", (Rr & 0xF)*2);
    char name_adiw[32]; sprintf(name_adiw,  "r%d", 24 + ((opcode & 0x30)>>3));
    char name_Ka[32];   sprintf(name_Ka,    "$%02X", Ka);
    char name_bit7[4];  sprintf(name_bit7,  "%d", Bit7);

    // Смещение
    char name_Yq[32];   if (Qi) sprintf(name_Yq, "Y+%d", Qi); else sprintf(name_Yq, "Y");
    char name_Zq[32];   if (Qi) sprintf(name_Zq, "Z+%d", Qi); else sprintf(name_Zq, "Z");
    char name_LDD[32];  if (Qi) sprintf(name_LDD, "ldd"); else sprintf(name_LDD, "ld");
    char name_STD[32];  if (Qi) sprintf(name_STD, "std"); else sprintf(name_STD, "st");

    // Условие
    char name_brc[16];  sprintf(name_brc, "%s", ds_brcs[0][Bit7]);
    char name_brs[16];  sprintf(name_brs, "%s", ds_brcs[1][Bit7]);

    // Вывод
    char mnem[32], op1[32], op2[32]; op1[0] = 0; op2[0] = 0; sprintf(mnem, "<unk>");

    // Store/Load indirect LDD, STD
    switch (opcode & 0xD208) {

        case 0x8000: strcpy(mnem, name_LDD); strcpy(op1, name_Rd); strcpy(op2, name_Zq); break;
        case 0x8008: strcpy(mnem, name_LDD); strcpy(op1, name_Rd); strcpy(op2, name_Yq); break;
        case 0x8200: strcpy(mnem, name_STD); strcpy(op1, name_Zq); strcpy(op2, name_Rd); break;
        case 0x8208: strcpy(mnem, name_STD); strcpy(op1, name_Yq); strcpy(op2, name_Rd); break;
    }

    // Immediate
    switch (opcode & 0xF000) {

        case 0xC000: strcpy(mnem, "rjmp");  strcpy(op1, name_rjmp); break; // k
        case 0xD000: strcpy(mnem, "rcall"); strcpy(op1, name_rjmp); break; // k
        case 0xE000: strcpy(mnem, "ldi");   strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
        case 0x3000: strcpy(mnem, "cpi");   strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
        case 0x4000: strcpy(mnem, "sbci");  strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
        case 0x5000: strcpy(mnem, "subi");  strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
        case 0x6000: strcpy(mnem, "ori");   strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
        case 0x7000: strcpy(mnem, "andi");  strcpy(op1, name_Rdi); strcpy(op2, name_K); break; // Rd, K
    }

    // lds/sts 16 bit, in/out
    switch (opcode & 0xF800) {

        case 0xB000: strcpy(mnem, "in");  strcpy(op1, name_Rd);  strcpy(op2, name_Ap);  break; // Rd, A
        case 0xB800: strcpy(mnem, "out"); strcpy(op1, name_Ap);  strcpy(op2, name_Rd);  break; // A, Rr
    }

    // bset/clr
    switch (opcode & 0xFF8F) {

        case 0x9408: strcpy(mnem, ds_brcs[2][ bit7s ]); break;
        case 0x9488: strcpy(mnem, ds_brcs[3][ bit7s ]); break;
    }

    // alu op1, op2
    switch (opcode & 0xFC00) {

        case 0x0400: strcpy(mnem, "cpc");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x0800: strcpy(mnem, "sbc");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x0C00: strcpy(mnem, "add");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x1C00: // Rd, Rr

            strcpy(mnem, Rd == Rr ? "rol" : "adc");
            strcpy(op1, name_Rd);
            if (Rr != Rd) { strcpy(op2, name_Rr); }
            break;

        case 0x2C00: strcpy(mnem, "mov");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x9C00: strcpy(mnem, "mul");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x1000: strcpy(mnem, "cpse"); strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x1400: strcpy(mnem, "cp");   strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x1800: strcpy(mnem, "sub");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x2000: strcpy(mnem, "and");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x2400: strcpy(mnem, "eor");  strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0x2800: strcpy(mnem, "or");   strcpy(op1, name_Rd); strcpy(op2, name_Rr); break; // Rd, Rr
        case 0xF000: sprintf(mnem, "br%s", name_brs); strcpy(op1, name_bjmp); break; // s, k
        case 0xF400: sprintf(mnem, "br%s", name_brc); strcpy(op1, name_bjmp); break; // s, k
    }

    // Bit operation
    switch (opcode & 0xFE08) {

        case 0xF800: strcpy(mnem, "bld");   strcpy(op1, name_Rd); strcpy(op2, name_bit7); break; // Rd, b
        case 0xFA00: strcpy(mnem, "bst");   strcpy(op1, name_Rd); strcpy(op2, name_bit7); break; // Rd, b
        case 0xFC00: strcpy(mnem, "sbrc");  strcpy(op1, name_Rd); strcpy(op2, name_bit7); break; // Rr, b
        case 0xFE00: strcpy(mnem, "sbrs");  strcpy(op1, name_Rd); strcpy(op2, name_bit7); break; // Rr, b
    }

    // jmp/call 24 bit
    switch (opcode & 0xFE0E) {

        case 0x940C: strcpy(mnem, "jmp");  append = ds_fetch(addr); sprintf(op1, "%05X", 2*(jmpfar + append)); break; // k2
        case 0x940E: strcpy(mnem, "call"); append = ds_fetch(addr); sprintf(op1, "%05X", 2*(jmpfar + append)); break; // k2
    }

    // ST/LD
    switch (opcode & 0xFE0F) {

        case 0x900F: strcpy(mnem, "pop");   strcpy(op1, name_Rd); break; // Rd
        case 0x920F: strcpy(mnem, "push");  strcpy(op1, name_Rd); break; // Rd
        case 0x940A: strcpy(mnem, "dec");   strcpy(op1, name_Rd); break; // Rd
        case 0x9204: strcpy(mnem, "xch");   strcpy(op1, name_Rd); break; // Rr
        case 0x9205: strcpy(mnem, "las");   strcpy(op1, name_Rd); break; // Rr
        case 0x9206: strcpy(mnem, "lac");   strcpy(op1, name_Rd); break; // Rr
        case 0x9207: strcpy(mnem, "lat");   strcpy(op1, name_Rd); break; // Rr
        case 0x9400: strcpy(mnem, "com");   strcpy(op1, name_Rd); break; // Rd
        case 0x9401: strcpy(mnem, "neg");   strcpy(op1, name_Rd); break; // Rd
        case 0x9402: strcpy(mnem, "swap");  strcpy(op1, name_Rd); break; // Rd
        case 0x9403: strcpy(mnem, "inc");   strcpy(op1, name_Rd); break; // Rd
        case 0x9405: strcpy(mnem, "asr");   strcpy(op1, name_Rd); break; // Rd
        case 0x9406: strcpy(mnem, "lsr");   strcpy(op1, name_Rd); break; // Rd
        case 0x9407: strcpy(mnem, "ror");   strcpy(op1, name_Rd); break; // Rd
        case 0x900A: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "-Y"); break; // Rd, -Y
        case 0x900C: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "X");  break; // Rd, X
        case 0x900D: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "X+"); break; // Rd, X+
        case 0x900E: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "-X"); break; // Rd, -X
        case 0x9001: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "Z+"); break; // Rd, Z+
        case 0x9002: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "-Z"); break; // Rd, -Z
        case 0x9004: strcpy(mnem, "lpm");   strcpy(op1, name_Rd); strcpy(op2, "Z");  break; // Rd, Z
        case 0x9005: strcpy(mnem, "lpm");   strcpy(op1, name_Rd); strcpy(op2, "Z+"); break; // Rd, Z+
        case 0x9006: strcpy(mnem, "elpm");  strcpy(op1, name_Rd); strcpy(op2, "Z");  break; // Rd, Z
        case 0x9007: strcpy(mnem, "elpm");  strcpy(op1, name_Rd); strcpy(op2, "Z+"); break; // Rd, Z+
        case 0x9009: strcpy(mnem, "ld");    strcpy(op1, name_Rd); strcpy(op2, "Y+"); break; // Rd, Y+
        case 0x920C: strcpy(mnem, "st");    strcpy(op1, "X");  strcpy(op2, name_Rd); break; // X, Rd
        case 0x920D: strcpy(mnem, "st");    strcpy(op1, "X+"); strcpy(op2, name_Rd); break; // X+, Rd
        case 0x920E: strcpy(mnem, "st");    strcpy(op1, "-X"); strcpy(op2, name_Rd); break; // -X, Rd
        case 0x9209: strcpy(mnem, "std");   strcpy(op1, "Y+"); strcpy(op2, name_Rd); break; // Y+, Rd
        case 0x920A: strcpy(mnem, "std");   strcpy(op1, "-Y"); strcpy(op2, name_Rd); break; // -Y, Rd
        case 0x9201: strcpy(mnem, "std");   strcpy(op1, "Z+"); strcpy(op2, name_Rd); break; // Z+, Rd
        case 0x9202: strcpy(mnem, "std");   strcpy(op1, "-Z"); strcpy(op2, name_Rd); break; // -Z, Rd

        case 0x9000:
        case 0x9200:

            strcpy(mnem, opcode & 0x0200 ? "sts" : "lds");
            strcpy(op1, name_Rd);
            sprintf(op2, "$%04X", ds_fetch(addr));
            break;
    }

    // Word Ops
    switch (opcode & 0xFF00) {

        case 0x0100: strcpy(mnem, "movw"); strcpy(op1, name_Rd4);  strcpy(op2, name_Rr4);  break; // Rd+1:Rd, Rr+1:Rr
        case 0x0200: strcpy(mnem, "muls"); strcpy(op1, name_Rd);   strcpy(op2, name_Rr);   break; // Rd, Rr
        case 0x9A00: strcpy(mnem, "sbi");  strcpy(op1, name_Ap8);  strcpy(op2, name_bit7); break; // A, b
        case 0x9B00: strcpy(mnem, "sbis"); strcpy(op1, name_Ap8);  strcpy(op2, name_bit7); break; // A, b
        case 0x9600: strcpy(mnem, "adiw"); strcpy(op1, name_adiw); strcpy(op2, name_Ka);   break; // Rd+1:Rd, K
        case 0x9700: strcpy(mnem, "sbiw"); strcpy(op1, name_adiw); strcpy(op2, name_Ka);   break; // Rd+1:Rd, K
        case 0x9800: strcpy(mnem, "cbi");  strcpy(op1, name_Ap8);  strcpy(op2, name_bit7); break; // A, b
        case 0x9900: strcpy(mnem, "sbic"); strcpy(op1, name_Ap8);  strcpy(op2, name_bit7); break; // A, b
    }

    // DES
    switch (opcode & 0xFF0F) {

        case 0x940B: strcpy(mnem, "des");  sprintf(op1, "$%02X", (opcode & 0xF0) >> 4); break; // K
    }

    // Multiply
    switch (opcode & 0xFF88) {

        case 0x0300: strcpy(mnem, "mulsu");  strcpy(op1, name_Rd); strcpy(op1, name_Rr); break; // Rd, Rr
        case 0x0308: strcpy(mnem, "fmul");   strcpy(op1, name_Rd); strcpy(op1, name_Rr); break; // Rd, Rr
        case 0x0380: strcpy(mnem, "fmuls");  strcpy(op1, name_Rd); strcpy(op1, name_Rr); break; // Rd, Rr
        case 0x0388: strcpy(mnem, "fmulsu"); strcpy(op1, name_Rd); strcpy(op1, name_Rr); break; // Rd, Rr
    }

    // Одиночные
    switch (opcode & 0xFFFF) {

        case 0x0000: strcpy(mnem, "nop");   break;
        case 0x95A8: strcpy(mnem, "wdr");   break;
        case 0x95C8: strcpy(mnem, "lpm");   strcpy(op1, "r0"); strcpy(op2, "Z"); break; // R0, Z
        case 0x95D8: strcpy(mnem, "elpm");  strcpy(op1, "r0"); strcpy(op2, "Z"); break; // R0, Z
        case 0x9409: strcpy(mnem, "ijmp");  break;
        case 0x9419: strcpy(mnem, "eijmp"); break;
        case 0x9508: strcpy(mnem, "ret");   break;
        case 0x9509: strcpy(mnem, "icall"); break;
        case 0x9518: strcpy(mnem, "reti");  break;
        case 0x9519: strcpy(mnem, "eicall"); break;
        case 0x95E8: strcpy(mnem, "spm");   break;
        case 0x95F8: strcpy(mnem, "spm.2"); break;
        case 0x9588: strcpy(mnem, "sleep"); break;
        case 0x9598: strcpy(mnem, "break"); break;
    }

    // Дополнить пробелами
    int l_mnem = strlen(mnem);
    int l_op1  = strlen(op1);
    int l_op2  = strlen(op2);

    for (int i = l_mnem; i < 8; i++) strcat(mnem, " ");

    // Вывести строку
    if (l_op1 && l_op2) {
        sprintf(ds_line, "%s %s, %s", mnem, op1, op2);
    } else if (l_op1) {
        sprintf(ds_line, "%s %s", mnem, op1);
    } else {
        sprintf(ds_line, "%s", mnem);
    }

    return addr - pvaddr;
}

// Обновить экран с дизассемблированием
void APP::ds_update() {

    int  size, cl, k;
    char tmp[32];
    int  catched = 0;

    int  wx = width  / 8;
    int  wy = height / 16;

    cls(0x000040);
    ds_current = ds_start;

    // Отрисовка дизассемблера
    for (int i = 1; i < wy-1; i++) {

        int hlight = 0;

        // Декодирование
        size = ds_info(ds_current);

        // Запись на всякий пожарный
        ds_addresses[i] = ds_current;

        // Нарисовать линию
        if (ds_cursor == ds_current) {

            // Показать только если табуляция тут есть
            if (ds_tab == 0) {

                for (int a = 0; a < 16; a++)
                for (int b = 8; b < 8*(wx-42); b++)
                    pset(b, i*16 + a, 0x0080ff);

                hlight = 1;
            }
            catched = 1;
        }

        // Адрес
        sprintf(tmp, "%05X", ds_current); print(2, i, tmp, hlight ? 0xffff00 : 0xe0e000);

        // Обычный размер (size = 4)
        sprintf(tmp, "%04X", program[ds_current] + 256*program[ds_current+1]); print(8, i, tmp, hlight ? 0xf0f0f0 : 0xa0a0a0);

        // Расширенный размер
        if (size == 4) {

            sprintf(tmp, "%04X", program[ds_current+2] + 256*program[ds_current+3]);
            print(13, i, tmp, hlight ? 0xf0f0f0 : 0xa0a0a0);
        }

        // Мнемоника и аргумент
        print(18, i, ds_line, hlight ? 0xffffff : 0xe0e0e0);

        // Точка текущего положения
        if (ds_current == pc) print(1, i, "\x10", 0xfff80);

        // Тест на брейкпоинты
        for (k = 0; k < ds_brk_cnt; k++) if (ds_current == ds_brk[k]) print(7, i, "\x0A", 0xff00000);

        // К следующей
        ds_current += size;
    }

    // Если курсор не отловлен, перерисовать заново с новой позиции
    if (catched == 0) { ds_start = ds_cursor; ds_update(); }

    // Рамки
    print16char(0,      0,      0xC9, 0xc0c0c0);
    print16char(0,      wy-1,   0xC8, 0xc0c0c0);
    print16char(wx-42,  0,      0xCB, 0xc0c0c0);
    print16char(wx-42,  wy-1,   0xCA, 0xc0c0c0);
    print16char(wx-1,   0,      0xBB, 0xc0c0c0);
    print16char(wx-1,   wy-1,   0xBC, 0xc0c0c0);

    for (int i = 1; i < wy-1;  i++) { print16char(0, i, 0xBA, 0xc0c0c0); print16char(wx-42, i, 0xBA, 0xc0c0c0); print16char(wx-1, i, 0xBA, 0xc0c0c0); }
    for (int i = 1; i < wx-42; i++) { print16char(i, 0, 0xCD, 0xc0c0c0); print16char(i, wy-1, 0xCD, 0xc0c0c0); }
    for (int i = wx-41; i < wx-1; i++) { print16char(i, 0, 0xCD, 0xc0c0c0); print16char(i, wy-1, 0xCD, 0xc0c0c0); }

    // Вывод дампа регистров
    print(wx-38, 1, " 0  1  2  3  4  5  6  7  8  9", 0x00c000);

    for (int i = 0; i < 4; i++)
    for (int j = 0; j < 10; j++) {

        int ai  = i*10 + j;
        print16char(wx-40, 2 + i, '0' + i, 0x00c000);

        // Допустимый регистр
        if (ai < 32) {

            sprintf(tmp, "%02X", sram[ai]);
            print(wx-38 + j*3, 2 + i, tmp, pvsram[ai] == sram[ai] ? 0xc0c0c0 : 0xffff80);
        }
    }

    // Дамп портов
    print(wx-25, 6, " 0  1  2  3  4  5  6  7", 0x00c000);
    for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++) {

        int ai = 32 + i*8 + j;
        sprintf(tmp, "%02X", sram[ai]);
        print(wx-25 + j*3, 7 + i, tmp, pvsram[ai] == sram[ai] ? 0xc0c0c0 : 0xffff80);

        sprintf(tmp, "%02X", i*8);
        print(wx-28, 7 + i, tmp, 0x00c000);
    }

    // Регистры
    sprintf(tmp, " X: %04X", get_X()); print(wx-40, 7, tmp, 0xc0c0c0);
    sprintf(tmp, " Y: %04X", get_Y()); print(wx-40, 8, tmp, 0xc0c0c0);
    sprintf(tmp, " Z: %04X", get_Z()); print(wx-40, 9, tmp, 0xc0c0c0);
    sprintf(tmp, "SP: %04X", get_S()); print(wx-40, 10, tmp, 0xc0c0c0);
    sprintf(tmp, "PC: %04X", pc);      print(wx-40, 11, tmp, 0xc0c0c0);
    sprintf(tmp, "%08X", instr_counter); print(wx-40, 12, tmp, 0x808080);

    // Флаги
    print16char(wx-40, 14, flag.i ? 'I' : 'i', flag.i ? 0x00ff00 : 0x808080);
    print16char(wx-39, 14, flag.t ? 'T' : 't', flag.t ? 0x00ff00 : 0x808080);
    print16char(wx-38, 14, flag.h ? 'H' : 'h', flag.h ? 0x00ff00 : 0x808080);
    print16char(wx-37, 14, flag.s ? 'S' : 's', flag.s ? 0x00ff00 : 0x808080);
    print16char(wx-36, 14, flag.v ? 'V' : 'v', flag.v ? 0x00ff00 : 0x808080);
    print16char(wx-35, 14, flag.n ? 'N' : 'n', flag.n ? 0x00ff00 : 0x808080);
    print16char(wx-34, 14, flag.z ? 'Z' : 'z', flag.z ? 0x00ff00 : 0x808080);
    print16char(wx-33, 14, flag.c ? 'C' : 'c', flag.c ? 0x00ff00 : 0x808080);

    // Память
    print(wx-35, 16, " 0 1 2 3 4 5 6 7 8 9 A B C D E F", 0x00c000);

    ds_dump_cursor &= 0xffff;
    ds_dump_start  &= 0xffff;

    for (int i = 0; i < 27; i++) {

        sprintf(tmp, "%04X", (ds_dump_start + 16*i) & 0xffff);

        cl = 0xf0f0c0;
        if (ds_tab == 1 && (ds_dump_cursor & 0xFFF0) == (ds_dump_start + 16*i))
            cl = 0x00ff00;

        // Печать текущего адреса
        print(wx-40, 17 + i, tmp, cl);

        // Печать дампа
        for (int j = 0; j < 16; j++) {

            int ai = ds_dump_start + ((16*i + j) & 0xffff);
            sprintf(tmp, "%02X", sram[ai]);

            print(wx-40 + 5 + 2*j, 17 + i, tmp, j % 2 ? 0xa0f0a0 : 0xa0a0a0);
        }
    }

    // Сохранить, чтобы вычислять изменения
    for (int i = 0; i < 96; i++) pvsram[i] = sram[i];
}
