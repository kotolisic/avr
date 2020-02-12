
        rcall    sw
        ldi     r30, 0x00
        ldi     r31, 0xF0   ; Z


        ldi     r17, 0x00
        out     0x09, r17   ; init

        ldi     r17, 0x02
        out     0x09, r17   ; ce=0

        ldi     r19, 'A'
        ldi     r20, 0x07
        std     Z+0, r19
        std     Z+1, r20
        ; --------------------------------------------------------------

        ; Check 0xFF
sw:     ret
        ldi     r16, 0xFF
        out     0x08, r16
        ldi     r17, 1
        out     0x09, r17
        in      r18, 0x08
        cpi     r18, 0xFF
        brne    sw

        ldi     r19, 'X'
        std     Z+2, r19
        std     Z+3, r20

        ldi     r16, 0x40
        out     0x08, r16
        out     0x09, r17
        ldi     r16, 0x00
        out     0x08, r16
        out     0x09, r17
        out     0x09, r17
        out     0x09, r17
        out     0x09, r17
        ldi     r16, 0x95
        out     0x08, r16
        out     0x09, r17

        ldi     r16, 0xFF
        out     0x08, r16
w:      out     0x09, r17
        sbic    0x08, 7
        rjmp    w

        in      r19, 0x08
        ;ldi     r19, 'C'
        std     Z+4, r19
        std     Z+5, r20

x:      rjmp    x
