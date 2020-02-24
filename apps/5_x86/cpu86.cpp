#include <avrio.c>

#include "bios.h"
#include "cpu86.h"

class CPU86 {

protected:

    word    ax, bx, cx, dx, sp, bp, si, di;
    word    es, cs, ss, ds;
    word    flags;
    word    ip;

    byte    modrm, modrm_reg, modrm_rm, modrm_mod;
    byte    prefix_mask;
    word    segment, segment_over, effective;
    word    op1, op2;

public:

    // Одна инструкция
    void step() {

        cs = 0xff00;
        ip = 0;

        prefix_mask  = 0;
        segment_over = 0;
        segment      = ds;

        switch (fetch_byte() & 7) {

            /* ADD rm, r8  */ case 0x00: fetch_modrm(); break;
            /* ADD rm, r16 */ case 0x01: break;
            /* ADD r8, rm  */ case 0x02: break;
            /* ADD r16, rm */ case 0x03: break;
            /* ADD al, i8  */ case 0x04: break;
            /* ADD ax, i16 */ case 0x05: break;
            /* PUSH es     */ case 0x06: break;
            /* POP  es     */ case 0x07: break;
        }
    }

    // -----------------------------------------------------------------

    // Чтение байта из памяти
    byte read_byte(word segment, word offset) {

        heap(vm, 0xF000);

        dword address = ((dword)segment<<4) | offset;

        // ROM:BIOS (4kb)
        if (address >= 0xFF000 && address <= 0xFFFFF) {
            return BIOSROM[address & 0xFFF];
        }
        // Текстовая видеопамять
        else if (address >= 0xB8000 && address < 0xB8FA0) {
            return vm[address & 0xFFF];
        }

        // Общая память
        return dram.read(address);
    }

    // Прочесть следующий байт
    byte fetch_byte() { return read_byte(cs, ip++); }

    // Прочесть слово из памяти
    word fetch_word() {

        byte l = fetch_byte();
        return (fetch_byte()<<8) + l;
    }

    // Раскодировка байта modrm
    void fetch_modrm() {

        int d;

        modrm = fetch_byte();
        modrm_reg = (modrm & 0x38) >> 3;
        modrm_rm  = (modrm & 0x07);
        modrm_mod = (modrm & 0xc0);

        // Раскодирование смещения или регистра
        switch (modrm_mod) {

            case 0x00:

                effective = modrm_rm == 6 ? fetch_word() : decode_effective();
                break;

            case 0x40:

                d = fetch_byte();
                d = d & 0x80 ? d - 256 : d;
                effective = decode_effective() + d;
                break;

            case 0x80:

                d = fetch_word();
                d = d & 0x8000 ? d - 65536 : d;
                effective = decode_effective() + d;
                break;
        }

        // if seg_override, segment = segment_over
    }

    word decode_effective() {

        switch (modrm_rm) {

            case 0: return bx + si;
            case 1: return bx + di;
            case 2: segment = ss; return bp + si;
            case 3: segment = ss; return bp + di;
            case 4: return si;
            case 5: return di;
            case 6: segment = ss; return bp;
            case 7: return bx;
        }

        return 0;
    }

    // Значение регистра
    byte get_r8(byte id) {

        switch (id) {

            case 0: return ax & 0xFF;
            case 1: return cx & 0xFF;
            case 2: return dx & 0xFF;
            case 3: return bx & 0xFF;
            case 4: return (ax >> 8) & 0xFF;
            case 5: return (cx >> 8) & 0xFF;
            case 6: return (dx >> 8) & 0xFF;
            case 7: return (bx >> 8) & 0xFF;
        }

    }

    // Значение RM-части modrm
    byte get_rm() {

        if (modrm_mod == 0xc0)
             return get_r8(modrm_rm);
        else return read_byte(segment, effective);
    }
};
