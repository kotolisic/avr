#include "avr.h"

// Чтение из памяти
unsigned char APP::get(int addr) {

    addr &= 0xFFFF;

    // Банкинг памяти
    if (addr >= 0xF000) addr += 4096 * membank;

    unsigned char dv = 0;

    // Очистка бита в порту 00 при чтении
    switch (addr) {

        // Банк памяти
        case 0x20: return membank;
        case 0x21: return 0;

        // Клавиатура
        case 0x22: return port_keyb_xt;
        case 0x23: dv = port_keyb_hit; port_keyb_hit &= ~1; break;

        // Курсор
        case 0x24: dv = cursor_x; break;
        case 0x25: dv = cursor_y; break;

        // Таймер
        case 0x26: dv = timer & 0xff; break;
        case 0x27: dv = (timer >> 8) & 0xff; break;

        // SPI
        case 0x28: dv = spi_read_data(); break;
        case 0x29: dv = spi_read_status(); break;

        // Мышь
        case 0x2A: dv =  get_mouse_x();       break; // Lo Mouse
        case 0x2E: dv = (get_mouse_x() >> 8); break; // Hi Mouse
        case 0x2B: dv =  get_mouse_y();       break;
        case 0x2C: dv = mouse_cmd; break;

        // Видеорежим
        case 0x2D: return videomode;

        // Остальная память
        default:   dv = sram[addr]; break;
    }

    return dv & 0xFF;
}

// Сохранение в память
void APP::put(int addr, unsigned char value) {

    addr &= 0xFFFF;

    // Учитывать банкинг
    if (addr >= 0xF000) addr += 4096 * membank;

    sram[addr] = value;

    // Запись во флаги
    if (addr == 0x5F) byte_to_flag(value);

    // Карта памяти
    if (addr == 0x20) { membank = value; }
    if (addr == 0x21) { /* membank */ }
    if (addr == 0x22) { port_keyb_xt  = value; }
    if (addr == 0x23) { port_keyb_hit = value; }

    // Обновление позиции курсора
    if (addr == 0x24) { cursor_x = value; update_text_xy(text_px, text_py); update_text_xy(cursor_x, cursor_y); }
    if (addr == 0x25) { cursor_y = value; update_text_xy(text_px, text_py); update_text_xy(cursor_x, cursor_y); }

    if (addr == 0x28) { spi_write_data(value); }
    if (addr == 0x29) { spi_write_cmd(value); }
    if (addr == 0x2D) { videomode = value; update_screen(); }

    // Нарисовать на холсте
    if (addr >= 0xC000) { update_byte_scr(addr); }
}

// Байт во флаги
void APP::byte_to_flag(unsigned char f) {

    flag.c = (f >> 0) & 1;
    flag.z = (f >> 1) & 1;
    flag.n = (f >> 2) & 1;
    flag.v = (f >> 3) & 1;
    flag.s = (f >> 4) & 1;
    flag.h = (f >> 5) & 1;
    flag.t = (f >> 6) & 1;
    flag.i = (f >> 7) & 1;
};

// Флаги в байты
unsigned char APP::flag_to_byte() {

    unsigned char f =
        (flag.i<<7) |
        (flag.t<<6) |
        (flag.h<<5) |
        (flag.s<<4) |
        (flag.v<<3) |
        (flag.n<<2) |
        (flag.z<<1) |
        (flag.c<<0);

    sram[0x5F] = f;
    return f;
}

// Развернуть итоговые биты
unsigned int APP::neg(unsigned int n) {
    return n ^ 0xffff;
}

// Установка флагов
void APP::set_logic_flags(unsigned char r) {

    flag.v = 0;
    flag.n = (r & 0x80) ? 1 : 0;
    flag.s = flag.n;
    flag.z = (r == 0) ? 1 : 0;
    flag_to_byte();
}

// Флаги после вычитания с переносом
void APP::set_subcarry_flag(int d, int r, int R, int carry) {

    flag.c = d < r + carry ? 1 : 0;
    flag.z = ((R & 0xFF) == 0 && flag.z) ? 1 : 0;
    flag.n = (R & 0x80) > 1 ? 1 : 0;
    flag.v = (((d & neg(r) & neg(R)) | (neg(d) & r & R)) & 0x80) > 0 ? 1 : 0;
    flag.s = flag.n ^ flag.v;
    flag.h = (((neg(d) & r) | (r & R) | (R & neg(d))) & 0x80) > 0 ? 1 : 0;
    flag_to_byte();
}

// Флаги после вычитания
void APP::set_subtract_flag(int d, int r, int R) {

    flag.c = d < r ? 1 : 0;
    flag.z = (R & 0xFF) == 0 ? 1 : 0;
    flag.n = (R & 0x80) > 1 ? 1 : 0;
    flag.v = (((d & neg(r) & neg(R)) | (neg(d) & r & R)) & 0x80) > 0 ? 1 : 0;
    flag.s = flag.n ^ flag.v;
    flag.h = (((neg(d) & r) | (r & R) | (R & neg(d))) & 0x40) > 0 ? 1 : 0;
    flag_to_byte();
}

// Флаги после сложение с переносом
void APP::set_add_flag(int d, int r, int R, int carry) {

    flag.c = d + r + carry >= 0x100;
    flag.h = (((d & r) | (r & neg(R)) | (neg(R) & d)) & 0x08) > 0 ? 1 : 0;
    flag.z = R == 0 ? 1 : 0;
    flag.n = (R & 0x80) > 0 ? 1 : 0;
    flag.v = (((d & r & neg(R)) | (neg(d) & neg(r) & R)) & 0x80) > 0 ? 1 : 0;
    flag.s = flag.n ^ flag.v;
    flag_to_byte();
}

// Флаги после логической операции сдвига
void APP::set_lsr_flag(int d, int r) {

    flag.c = d & 1;
    flag.n = (r & 0x80) > 0 ? 1 : 0;
    flag.z = (r == 0x00) ? 1 : 0;
    flag.v = flag.n ^ flag.c;
    flag.s = flag.n ^ flag.v;
    flag_to_byte();
}

// Флаги после сложения 16 бит
void APP::set_adiw_flag(int a, int r) {

    flag.v = ((neg(a) & r) & 0x8000) > 0 ? 1 : 0;
    flag.c = ((neg(r) & a) & 0x8000) > 0 ? 1 : 0;
    flag.n = (r & 0x8000) > 0 ? 1 : 0;
    flag.z = (r & 0xFFFF) == 0 ? 1 : 0;
    flag.s = flag.v ^ flag.n;
    flag_to_byte();
}

// Флаги после вычитания 16 бит
void APP::set_sbiw_flag(int a, int r) {

    flag.v = ((neg(a) & r) & 0x8000) > 0 ? 1 : 0;
    flag.c = ((neg(a) & r) & 0x8000) > 0 ? 1 : 0;
    flag.n = (r & 0x8000) > 0 ? 1 : 0;
    flag.z = (r & 0xFFFF) == 0 ? 1 : 0;
    flag.s = flag.v ^ flag.n;
    flag_to_byte();
}

// В зависимости от инструкции CALL/JMP/LDS/STS
int APP::skip_instr() {

    switch (map[ fetch() ]) {

        case CALL:
        case JMP:
        case LDS:
        case STS:
        
            pc += 2;
            break;
    }
    
    return 2;
}

// Исполнение шага процессора
int APP::step() {

    int R, r, d, a, b, A, v, Z;
    unsigned short p;

    cycles  = 1;
    opcode  = fetch();
    command = map[opcode];

    // Исполнение опкода
    switch (command) {

        case WDR:
        case NOP: break;

        // Остановка выполнения кода
        case SLEEP: /* pc = pc - 2; break; */
        case BREAK: cpu_halt = 1; break;

        // Управляющие
        case RJMP:  pc = get_rjmp(); cycles = 2; break;
        case RCALL: push16(pc >> 1); pc = get_rjmp(); cycles = 3; break;
        case RET:   pc = pop16() << 1; break;
        case RETI:  pc = pop16() << 1; flag.i = 1; flag_to_byte(); break;
        case BCLR:  sram[0x5F] &= ~(1 << get_s3()); byte_to_flag(sram[0x5F]); break;
        case BSET:  sram[0x5F] |=  (1 << get_s3()); byte_to_flag(sram[0x5F]); break;

        // Условные перехдоды
        case BRBS: if ( (sram[0x5F] & (1<<(opcode & 7)))) { pc = get_branch(); cycles = 2; } break;
        case BRBC: if (!(sram[0x5F] & (1<<(opcode & 7)))) { pc = get_branch(); cycles = 2; }  break;

        // --------------------------------
        case CPSE: if (get_rd() == get_rr())              cycles = skip_instr(); break;
        case SBRC: if (!(get_rd() & (1 << (opcode & 7)))) cycles = skip_instr(); break;
        case SBRS: if   (get_rd() & (1 << (opcode & 7)))  cycles = skip_instr(); break;
        case SBIS:
        case SBIC: // Пропуск, если в порту Ap есть бит (или нет бита)

            b = (opcode & 7);
            A = (opcode & 0xF8) >> 3;
            v = get(0x20 + A) & (1 << b);
            if ((command == SBIS && v) || (command == SBIC && !v)) cycles = skip_instr();
            break;
        // --------------------------------

        // Ввод-вывод
        case IN:  put_rd(get(0x20 + get_ap())); break;
        case OUT: put(0x20 + get_ap(), get_rd()); break;

        // Сброс/установка бита в I/O
        case CBI:
        case SBI:

            b = 1 << (opcode & 0x07);
            A = (opcode & 0xF8) >> 3;

            if (command == CBI)
                 put(0x20 + A, get(0x20 + A) & ~b);
            else put(0x20 + A, get(0x20 + A) |  b);

            cycles = 2;
            break;

        // Стек
        case PUSH: push8(get_rd()); cycles = 2; break;
        case POP:  put_rd(pop8()); cycles = 2; break;

        // Срециальные
        case SWAP: d = get_rd(); put_rd(((d & 0x0F) << 4) + ((d & 0xF0) >> 4)); break;
        case BST:  flag.t = (get_rd() & (1 << (opcode & 7))) > 0 ? 1 : 0; flag_to_byte(); break;
        case BLD:  a = get_rd(); b = (1 << (opcode & 7)); put_rd( flag.t ? (a | b) : (a & ~b) ); break;

        // =============================================================
        // Арифметико-логические инструкции
        // =============================================================

        // Вычитание с переносом, но без записи
        case CPC:

            d = get_rd();
            r = get_rr();
            R = (d - r - flag.c) & 0xff;
            set_subcarry_flag(d, r, R, flag.c);
            break;

        // Вычитание с переносом
        case SBC:

            d = get_rd();
            r = get_rr();
            R = (d - r - flag.c) & 0xFF;
            set_subcarry_flag(d, r, R, flag.c);
            put_rd(R);
            break;

        // Сложение без переноса
        case ADD:

            d = get_rd();
            r = get_rr();
            R = (d + r) & 0xff;
            set_add_flag(d, r, R, 0);
            put_rd(R);
            break;

        // Вычитание без переноса
        case CP:

            d = get_rd();
            r = get_rr();
            R = (d - r) & 0xFF;
            set_subtract_flag(d, r, R);
            break;

        // Вычитание без переноса
        case SUB:

            d = get_rd();
            r = get_rr();
            R = (d - r) & 0xFF;
            set_subtract_flag(d, r, R);
            put_rd(R);
            break;

        // Сложение с переносом
        case ADC:

            d = get_rd();
            r = get_rr();
            R = (d + r + flag.c) & 0xff;
            set_add_flag(d, r, R, flag.c);
            put_rd(R);
            break;

        case AND: R = get_rd() & get_rr(); set_logic_flags(R); put_rd(R); break;
        case OR:  R = get_rd() | get_rr(); set_logic_flags(R); put_rd(R); break;
        case EOR: R = get_rd() ^ get_rr(); set_logic_flags(R); put_rd(R); break;

        // Логическое умножение
        case ANDI: R = get_rdi() & get_imm8(); set_logic_flags(R); put_rdi(R); break;
        case ORI:  R = get_rdi() | get_imm8(); set_logic_flags(R); put_rdi(R); break;

        // Вычитание непосредственного значения
        case SUBI:

            d = get_rdi();
            r = get_imm8();
            R = (d - r) & 0xFF;
            set_subtract_flag(d, r, R);
            put_rdi(R);
            break;

        // Вычитание непосредственного значения с переносом
        case SBCI:

            d = get_rdi();
            r = get_imm8();
            R = (d - r - flag.c) & 0xFF;
            set_subcarry_flag(d, r, R, flag.c);
            put_rdi(R);
            break;

        // Сравнение без переноса
        case CPI:

            d = get_rdi();
            r = get_imm8();
            R = (d - r) & 0xFF;
            set_subtract_flag(d, r, R);
            break;

        // Развернуть биты в другую сторону
        case COM:

            d = get_rd();
            r = (d ^ 0xFF) & 0xFF;
            set_logic_flags(r);
            flag.c = 1; flag_to_byte();
            put_rd(r);
            break;

        // Декремент
        case DEC:

            d = get_rd();
            r = (d - 1) & 0xff;
            put_rd(r);

            flag.v = (r == 0x7F) ? 1 : 0;
            flag.n = (r & 0x80) > 0 ? 1 : 0;
            flag.z = (r == 0x00) ? 1 : 0;
            flag.s = flag.v ^ flag.n;
            flag_to_byte();
            break;

        // Инкремент
        case INC:

            d = get_rd();
            r = (d + 1) & 0xff;
            put_rd(r);

            flag.v = (r == 0x80) ? 1 : 0;
            flag.n = (r & 0x80) > 0 ? 1 : 0;
            flag.z = (r == 0x00) ? 1 : 0;
            flag.s = flag.v ^ flag.n;
            flag_to_byte();
            break;

        // Сложение 16-битного числа
        case ADIW:

            d = 24 + ((opcode & 0x30) >> 3);
            a = get16(d);
            r = a + get_ka();
            set_adiw_flag(a, r);
            put16(d, r);
            cycles = 2;
            break;

        // Вычитание 16-битного числа
        case SBIW:

            d = 24 + ((opcode & 0x30) >> 3);
            a = get16(d);
            r = a - get_ka();
            set_sbiw_flag(a, r);
            put16(d, r);
            cycles = 2;
            break;

        // Логический сдвиг вправо
        case LSR:

            d = get_rd();
            r = d >> 1;
            set_lsr_flag(d, r);
            put_rd(r);
            break;

        // Арифметический вправо
        case ASR:

            d = get_rd();
            r = (d >> 1) | (d & 0x80);
            set_lsr_flag(d, r);
            put_rd(r);
            break;

        // Циклический сдвиг вправо
        case ROR:

            d = get_rd();
            r = (d >> 1) | (flag.c ? 0x80 : 0x00);
            set_lsr_flag(d, r);
            put_rd(r);
            break;

        // Отрицание
        case NEG:

            d = get_rd();
            R = (-d) & 0xFF;
            set_subtract_flag(0, d, R);
            put_rd(R);
            break;

        // =============================================================
        // Перемещения
        // =============================================================
        case LDI: put_rdi(get_imm8()); break;

        // Загрузка из памяти в регистры
        case LPM0Z:  sram[0] = program[get_Z()]; cycles = 3; break;
        case LPMRZ:  put_rd(program[get_Z()]); cycles = 3; break;
        case LPMRZ_: p = get_Z(); put_rd(program[p]); put_Z(p+1); cycles = 3; break;

        // Store X
        case STX:   put(get_X(), get_rd()); cycles = 2; break;
        case STX_:  p = get_X();     put(p, get_rd()); put_X(p+1); cycles = 2; break;
        case ST_X:  p = get_X() - 1; put(p, get_rd()); put_X(p); cycles = 2; break;

        // Store Y
        case STYQ:  put((get_Y() + get_qi()), get_rd()); cycles = 2; break;
        case STY_:  p = get_Y();     put(p, get_rd()); put_Y(p+1); cycles = 2; break;
        case ST_Y:  p = get_Y() - 1; put(p, get_rd()); put_Y(p); cycles = 2; break;

        // Store Z
        case STZQ:  put((get_Z() + get_qi()), get_rd()); cycles = 2; break;
        case STZ_:  p = get_Z();     put(p, get_rd()); put_Z(p+1); cycles = 2; break;
        case ST_Z:  p = get_Z() - 1; put(p, get_rd()); put_Z(p); cycles = 2; break;

        // Load X
        case LDX:   put_rd(get(get_X())); break;
        case LDX_:  p = get_X();     put_rd(get(p)); put_X(p+1); break;
        case LD_X:  p = get_X() - 1; put_rd(get(p)); put_X(p); break;

        // Load Y
        case LDYQ:  put_rd(get((get_Y() + get_qi()))); cycles = 2; break;
        case LDY_:  p = get_Y();     put_rd(get(p)); put_Y(p+1); cycles = 2; break;
        case LD_Y:  p = get_Y() - 1; put_rd(get(p)); put_Y(p); cycles = 2; break;

        // Load Z
        case LDZQ:  put_rd(get((get_Z() + get_qi()))); cycles = 2; break;
        case LDZ_:  p = get_Z();     put_rd(get(p)); put_Z(p+1); cycles = 2; break;
        case LD_Z:  p = get_Z() - 1; put_rd(get(p)); put_Z(p); cycles = 2; break;

        case MOV:   put_rd(get_rr()); break;
        case MOVW:

            r = (get_rr_index() & 0xF) << 1;
            d = (get_rd_index() & 0xF) << 1;

            put16(d, get16(r));
            break;

        case LDS: d = fetch(); put_rd( get(d) ); break;
        case STS: d = fetch(); put(d, get_rd()); break;

        // Загрузка из доп. памяти
        case ELPM0Z:  sram[0] = program[ get_Z() + (sram[0x5B] << 16) ]; cycles = 3; break; break;
        case ELPMRZ:  put_rd(program[ get_Z() + (sram[0x5B] << 16) ]); break;
        case ELPMRZ_: p = get_Z() + (sram[0x5B] << 16); put_rd(program[p]); put_Z(p+1); cycles = 3; break;

        // ------------ РАСШИРЕНИЯ -------------------------------------

        /*
        // Логические операции между (Z) и Rd
        case LAC:

            Z = get_Z();
            put(Z, get(Z) & (get_rd() ^ 0xFF));
            break;

        case LAS:

            Z = get_Z();
            put(Z, get(Z) | get_rd());
            break;

        case LAT:

            Z = get_Z();
            put(Z, get(Z) ^ get_rd());
            break;

        // Обмен (Z) и Rd
        case XCH:

            p = get_Z();
            r = get(p);
            put(p, get_rd());
            put_rd(r);
            break;
        */

        case JMP:

            pc = 2 * ((get_jmp() << 16) | fetch());
            cycles = 3;
            break;

        case CALL:

            push16((pc + 2) >> 1);
            pc = 2 * ((get_jmp() << 16) | fetch());
            cycles = 4;
            break;

        default:

            printf("Неизвестная инструкция $%04x в pc=$%04x\n", opcode, pc - 2);
            exit(1);
    }

    instr_counter += cycles;
    return cycles;
}
