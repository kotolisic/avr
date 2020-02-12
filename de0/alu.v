// Режим работы АЛУ
// ---------------------------------------------------------------------
// 0 LDI    9  EOR      11 LSR
// 1 CPC    A  OR       12 ROR
// 2 SBC    B  <SREG>   13 DEC
// 3 ADD    C  COM      14 ADIW
// 5 CP     D  NEG      15 SBIW
// 6 SUB    E  SWAP
// 7 ADC    F  INC
// 8 AND    10 ASR
// ---------------------------------------------------------------------

module alu(

    // Ввод
    input wire [4:0]  mode,         // режим
    input wire [7:0]  d,            // dst
    input wire [7:0]  r,            // src
    input wire [7:0]  s,            // входящий s

    // Вывод
    output reg [7:0]  R,            // результат
    output reg [7:0]  S,            // s (новый)

    // 16 bit
    input wire [15:0] op1w,
    output reg [15:0] resw
);

// Вычисления
wire [7:0] sub = d - r;
wire [7:0] add = d + r;
wire [8:0] sbc = d - r - s[0];
wire [7:0] adc = d + r + s[0];
wire [7:0] lsr = {1'b0, d[7:1]};
wire [7:0] ror = {s[0], d[7:1]};
wire [7:0] asr = {d[7], d[7:1]};
wire [7:0] neg = -d;
wire [7:0] inc = d + 1;
wire [7:0] dec = d - 1;
wire [7:0] com = d ^ 8'hFF;
wire [7:0] swap = {d[3:0], d[7:4]};
reg        carry;

// 16 битные вычисления
wire [15:0] adiw = op1w + r;
wire [15:0] sbiw = op1w - r;

// Флаги переполнения после сложения и вычитания
wire add_flag_v = (d[7] &  r[7] & !R[7]) | (!d[7] & !r[7] & R[7]);
wire sub_flag_v = (d[7] & !r[7] & !R[7]) | (!d[7] &  r[7] & R[7]);
wire neg_flag_v = R == 8'h80;

// Флаги половинного переполнения после сложения и вычитания
wire add_flag_h = ( d[3] & r[3]) | (r[3] & !R[3]) | (!R[3] &  d[3]);
wire sub_flag_h = (!d[3] & r[3]) | (r[3] &  R[3]) | ( R[3] & !d[3]);
wire neg_flag_h = d[3] | (d[3] & R[3]) | R[3];

// Флаги ADIW, SBIW
wire adiw_v = !op1w[15] & resw[15];
wire adiw_c = !resw[15] & op1w[15];

// Логические флаги
wire [7:0] set_logic_flag = {
    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ R[7],
    /* v */ 1'b0,
    /* n */ R[7],
    /* z */ R[7:0] == 0,
    /* c */ s[0]
};

// Флаги после вычитания с переносом
wire [7:0] set_subcarry_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ sub_flag_h,
    /* s */ sub_flag_v ^ R[7],
    /* v */ sub_flag_v,
    /* n */ R[7],
    /* z */ (R[7:0] == 0) & s[1],
    /* c */ sbc[8]
};

// Флаги после вычитания
wire [7:0] set_subtract_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ sub_flag_h,
    /* s */ sub_flag_v ^ R[7],
    /* v */ sub_flag_v,
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ d < r
};

// Флаги после COM
wire [7:0] set_com_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ R[7],
    /* v */ 1'b0,
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ 1'b1
};

// Флаги после NEG
wire [7:0] set_neg_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ neg_flag_h,
    /* s */ neg_flag_v ^ R[7],
    /* v */ neg_flag_v,
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ d != 0
};

// Флаги после сложения с переносом
wire [7:0] set_add_flag = {
    /* i */ s[7],
    /* t */ s[6],
    /* h */ add_flag_h,
    /* s */ add_flag_v ^ R[7],
    /* v */ add_flag_v,
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ d + r + carry >= 9'h100
};

// Флаги после логической операции сдвига вправо
wire [7:0] set_lsr_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ d[0],
    /* v */ R[7] ^ d[0],
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ d[0]
};

// Флаги после INC
wire [7:0] set_inc_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ (R == 8'h80) ^ R[7],
    /* v */ (R == 8'h80),
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ s[0]
};

// Флаги после DEC
wire [7:0] set_dec_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ (R == 8'h7F) ^ R[7],
    /* v */ (R == 8'h7F),
    /* n */ R[7],
    /* z */ (R[7:0] == 0),
    /* c */ s[0]
};

// Флаги после ADIW
wire [7:0] set_adiw_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ adiw_v ^ resw[15],
    /* v */ adiw_v,
    /* n */ resw[15],
    /* z */ (resw[15:0] == 0),
    /* c */ adiw_c
};

// Флаги после SBIW
wire [7:0] set_sbiw_flag = {

    /* i */ s[7],
    /* t */ s[6],
    /* h */ s[5],
    /* s */ adiw_v ^ resw[15],
    /* v */ adiw_v,
    /* n */ resw[15],
    /* z */ (resw[15:0] == 0),
    /* c */ adiw_v
};

always @(*) begin

    S = s;
    carry = 0;

    case (mode)

        /* LDI  */ 0:  begin R = r; end
        /* CPC  */ 1:  begin R = sbc[7:0]; S = set_subcarry_flag; end
        /* SBC  */ 2:  begin R = sbc[7:0]; S = set_subcarry_flag; end
        /* ADD  */ 3:  begin R = add;      S = set_add_flag;      carry = 0; end
        /* CP   */ 5:  begin R = sub;      S = set_subtract_flag; end
        /* SUB  */ 6:  begin R = sub;      S = set_subtract_flag; end
        /* ADC  */ 7:  begin R = adc;      S = set_add_flag;      carry = s[0]; end
        /* AND  */ 8:  begin R = d & r;    S = set_logic_flag; end
        /* EOR  */ 9:  begin R = d ^ r;    S = set_logic_flag; end
        /* OR   */ 10: begin R = d | r;    S = set_logic_flag; end
        /* SREG */ 11: begin S = r;        end
        /* COM  */ 12: begin R = com;      S = set_com_flag; end
        /* NEG  */ 13: begin R = neg;      S = set_neg_flag; end
        /* SWAP */ 14: begin R = swap;     end
        /* INC  */ 15: begin R = inc;      S = set_inc_flag; end
        /* ASR  */ 16: begin R = asr;      S = set_lsr_flag; end
        /* LSR  */ 17: begin R = lsr;      S = set_lsr_flag; end
        /* ROR  */ 18: begin R = ror;      S = set_lsr_flag; end
        /* DEC  */ 19: begin R = dec;      S = set_dec_flag; end
        /* ADIW */ 20: begin resw = adiw;  S = set_adiw_flag; end
        /* SBIW */ 21: begin resw = sbiw;  S = set_sbiw_flag; end
        /* BLD  */ 22: begin

            case (r[2:0])
                0: R = {d[7:1], s[6]};
                1: R = {d[7:2], s[6], d[0]};
                2: R = {d[7:3], s[6], d[1:0]};
                3: R = {d[7:4], s[6], d[2:0]};
                4: R = {d[7:5], s[6], d[3:0]};
                5: R = {d[7:6], s[6], d[4:0]};
                6: R = {d[  7], s[6], d[5:0]};
                7: R = {        s[6], d[6:0]};
            endcase

        end

        default: R = 8'hFF;

    endcase

end

endmodule
