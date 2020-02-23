
        ldi     r16, 0x00
        out     0x10, r16
        out     0x11, r16
        out     0x12, r16
        out     0x13, r16
nok:    sbis    0x15, 0
        rjmp    nok
        in      r17, 0x14

