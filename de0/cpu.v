module cpu
(
    input  wire        clock,

    // Программная память
    output reg  [15:0] pc,          // Программный счетчик
    input  wire [15:0] ir,          // Инструкция из памяти

    // Оперативная память
    output reg  [15:0] address,     // Указатель на память RAM (sram)
    input  wire [ 7:0] din_raw,     // memory[ address ]
    output reg  [ 7:0] wb,          // Запись в память по address
    output reg         w,           // Разрешение записи в память
    output reg  [ 7:0] bank,        // Банк памяти
    output reg  [ 1:0] vmode,       // Видеорежим

    // Ввод-вывод
    input  wire [ 7:0] kb_ch,       // Клавиатура
    input  wire        kb_tr,       // Триггер внешний
    output wire        kb_hit,

    // Положение курсора
    output reg  [ 7:0] cursor_x,
    output reg  [ 7:0] cursor_y,

    // Мышь
    input  wire [ 8:0] mouse_x,
    input  wire [ 7:0] mouse_y,
    input  wire [ 1:0] mouse_cmd,

    // SPI
    output reg         spi_sent,     // =1 Сообщение отослано на spi
    output reg  [ 1:0] spi_cmd,     // Команда
    input  wire [ 7:0] spi_din,     // Принятое сообщение
    output reg  [ 7:0] spi_out,     // Сообщение на отправку
    input  wire [ 1:0] spi_st,      // bit 1: timeout (1); bit 0: busy

    // SDRAM
    output reg  [31:0] sdram_address,
    input  wire [ 7:0] sdram_i_data,
    output reg  [ 7:0] sdram_o_data,
    input  wire [ 7:0] sdram_status,
    output reg  [ 7:0] sdram_control
);

initial begin

    pc = 0; address = 0; wb = 0; w = 0; vmode = 0;

    sdram_address = 0;
    sdram_control = 0;
    sdram_o_data  = 0;

    cursor_x = 0;
    cursor_y = 0;
    spi_cmd  = 0;
    spi_out  = 0;
    spi_sent = 0;
    bank     = 0;

    // Low
    r[0] = 8'h00; r[4] = 8'h00; r[8]  = 8'h00; r[12] = 8'h01;
    r[1] = 8'h00; r[5] = 8'h00; r[9]  = 8'h00; r[13] = 8'h00;
    r[2] = 8'h00; r[6] = 8'h00; r[10] = 8'h00; r[14] = 8'h00;
    r[3] = 8'h00; r[7] = 8'h00; r[11] = 8'h00; r[15] = 8'h00;

    // High
    r[16] = 8'h00; r[20] = 8'h00; r[24] = 8'h00; r[28] = 8'h00;
    r[17] = 8'h00; r[21] = 8'h00; r[25] = 8'h00; r[29] = 8'h00;
    r[18] = 8'h00; r[22] = 8'h00; r[26] = 8'h00; r[30] = 8'h05;
    r[19] = 8'h00; r[23] = 8'h00; r[27] = 8'h00; r[31] = 8'hFA;

end

wire [7:0] _r1 = r[16];
wire [7:0] _r2 = r[17];
wire [7:0] _r3 = r[14];
wire [7:0] _r4 = r[15];

`define SPDEC 1
`define SPINC 2

// ---------------------------------------------------------------------
// Проксирование памяти DIN
// ---------------------------------------------------------------------

reg [7:0] din;

always @* begin

    casex (address)

        // Регистры
        16'b0000_0000_000x_xxxx: din = r[ address[4:0] ];

        // Банки
        16'h0020: din = bank;
        16'h0021: din = 8'h00;

        // Клавиатура
        16'h0022: din = kb_ch;
        16'h0023: din = {7'h0, kb_hit};

        // Курсор
        16'h0024: din = cursor_x;
        16'h0025: din = cursor_y;

        // Таймер
        16'h0026: din = timer_ms[ 7:0];
        16'h0027: din = timer_ms[15:8];
        16'h002F: din = timer_ms[23:16];

        // SPI
        16'h0028: din = spi_din;
        16'h0029: din = spi_st;

        // Мышь
        16'h002A: din = mouse_x[7:0];
        16'h002B: din = mouse_y[7:0];
        16'h002C: din = {6'h0, mouse_cmd[1:0]};
        16'h002E: din = {7'h0, mouse_x[8]};

        // Процессор
        16'h005B: din = rampz;
        16'h005D: din = sp[ 7:0];
        16'h005E: din = sp[15:8];
        16'h005F: din = sreg;

        // SDRAM
        16'h0030: din = sdram_address[ 7:0];
        16'h0031: din = sdram_address[15:8];
        16'h0032: din = sdram_address[23:16];
        16'h0033: din = sdram_address[31:24];
        16'h0034: din = sdram_i_data;
        16'h0035: din = sdram_status;

        // Память
        default:  din = din_raw;

    endcase

end

// ---------------------------------------------------------------------
// Регистры процессора, в том числе системные
// ---------------------------------------------------------------------

// Регистры
reg [ 7:0]  r[32];                      // Регистры
reg [ 1:0]  tstate  = 0;                // Машинное состояние
reg [15:0]  latch   = 0;                // Записанный опкод
reg [15:0]  pclatch = 0;                // Для LPM / SPM
reg [15:0]  sp      = 16'hEFFF;         // Stack Pointer
reg [ 7:0]  sreg    = 8'b0000_0000;     // Status Register
reg [ 4:0]  alu     = 0;                // Режим работы АЛУ
reg         locked  = 1'b0;             // Блокировать процессор, пока SPI занят
reg         spi_proc = 0;               // Процессор сейчас работает с SPI

// Команды на обратном фронте
reg         reg_w   = 0;                // Писать регистр
reg [ 4:0]  reg_id  = 0;                // В какой регистр писать
reg         sreg_w  = 0;                // Писать в SREG из АЛУ
reg [ 1:0]  sp_mth  = 0;                // Увеличение или уменьшение стека
reg         aread   = 0;                // Признак чтения из памяти
reg         kbit    = 0;                // Прочитал ли бит KBHIT

// 16 битные регистры
reg         reg_ww  = 0;                // Писать в X,Y,Z
reg         reg_ws  = 0;                // =1 Источник АЛУ; =0 Источник регистр `wb2`
reg [ 1:0]  reg_idw = 0;                // Номер 16-битного регистра
reg [15:0]  wb2     = 0;                // Что писать в X,Y,Z
reg [ 7:0]  rampz   = 0;                // Верхняя память для E-функции

// Провода
wire [15:0] opcode = tstate ? latch : ir;
wire [15:0] X   = {r[27], r[26]};
wire [15:0] Y   = {r[29], r[28]};
wire [15:0] Z   = {r[31], r[30]};
wire [15:0] Xm  = X - 1;
wire [15:0] Xp  = X + 1;
wire [15:0] Ym  = Y - 1;
wire [15:0] Yp  = Y + 1;
wire [15:0] Zm  = Z - 1;
wire [15:0] Zp  = Z + 1;
wire [ 5:0] q   = {opcode[13], opcode[11:10], opcode[2:0]};
wire [ 4:0] rd  =  opcode[8:4];
wire [ 4:0] rr  = {opcode[9], opcode[3:0]};
wire [ 4:0] rdi = {1'b1, opcode[7:4]};
wire [ 4:0] rri = {1'b1, opcode[3:0]};
wire [ 7:0] K   = {opcode[11:8], opcode[3:0]};

reg         skip_instr = 0;
wire [15:0] pcnext  = pc + 1;
wire [15:0] pcnext2 = pc + 2;
wire        is_call = {opcode[14], opcode[3:1]} == 4'b0111;

// Текущий статус
assign      kb_hit      = kb_trigger ^ kb_tr;
reg         kb_trigger  = 1'b0;

// Арифметико-логическое устройство
reg  [ 7:0] op1;
reg  [ 7:0] op2;
wire [ 7:0] alu_res;
wire [ 7:0] alu_sreg;
reg  [15:0] op1w;
wire [15:0] resw;

alu UnitALU
(
    alu,
    op1, op2, sreg,
    alu_res, alu_sreg,
    op1w, resw
);

// Таймер
reg [14:0] timer_divider = 0;
reg [23:0] timer_ms      = 0;

// Отсчет таймера в миллисекундах
always @(posedge clock) begin

    if (timer_divider == 24999) begin
        timer_divider <= 0;
        timer_ms      <= timer_ms + 1;
    end else
        timer_divider <= timer_divider + 1;
end

// Обнаружение posedge kbhit для вызова прерывания

// Исполнительное устройство
always @(posedge clock) begin
if (!locked) begin

    w      <= 1'b0;
    aread  <= 1'b0;
    reg_w  <= 1'b0;
    sreg_w <= 1'b0;
    sp_mth <= 1'b0; // Ничего не делать с SP
    reg_ww <= 1'b0; // Ничего не делать с X,Y,Z
    reg_ws <= 1'b0; // Источник регистр wb2

    if (tstate == 0) latch <= ir;

    // Код пропуска инструкции (JMP, CALL, LDS, STS)
    if (skip_instr) begin

        casex (opcode)

            16'b1001_010x_xxxx_11xx, // CALL | JMP
            16'b1001_00xx_xxxx_0000: // LDS  | STS
                pc <= pcnext + 1;
            default:
                pc <= pcnext;

        endcase

        skip_instr <= 0;

    end

    // Вызов прерывания
    // else if (tstate == 0 && intr)
    //  intr <= 0;
    //  .. call

    // Исполнение опкодов
    else casex (opcode)

        // NOP, BREAK
        16'b0000_0000_0000_0000: pc <= pcnext;
        16'b1001_0101_1001_1000: pc <= pcnext;

        // [1T] LDI Rd, K
        16'b1110_xxxx_xxxx_xxxx: begin

            pc      <= pcnext;
            alu     <= 0;       // Инструкция LDI
            op2     <= K;
            reg_w   <= 1'b1;
            reg_id  <= rdi;

        end

        // [1T] RJMP k | IJMP | EIJMP
        16'b1100_xxxx_xxxx_xxxx: pc <= pcnext + {{4{opcode[11]}}, opcode[11:0]};
        16'b1001_0100_000x_1001: pc <= Z;

        // [2T] JMP k
        16'b1001_010x_xxxx_110x:
        case (tstate)

            0: begin tstate <= 1; pc <= pcnext; end
            1: begin tstate <= 0; pc <= ir; end

        endcase

        // [1T] <ALU> Rd, Rr
        16'b000x_01xx_xxxx_xxxx, // 1 CPC, 5 CP
        16'b000x_1xxx_xxxx_xxxx, // 2 SBC, 3 ADD, 6 SUB, 7 ADC
        16'b0010_0xxx_xxxx_xxxx, // 8 AND, 9 EOR
        16'b0010_10xx_xxxx_xxxx: // A OR
        begin

            pc      <= pcnext;
            alu     <= opcode[13:10];
            op1     <= r[rd];
            op2     <= r[rr];
            // CP, CPС не писать
            reg_id  <= rd;
            reg_w   <= (opcode[13:10] != 4'b0001 && opcode[13:10] != 4'b0101);
            sreg_w  <= 1'b1;

        end

        // [1T] BRB[C/S] k
        16'b1111_0xxx_xxxx_xxxx:
        begin

            if (sreg[ opcode[2:0] ] ^ opcode[10])
                pc <= pcnext + {{9{opcode[9]}}, opcode[9:3]};
            else
                pc <= pcnext;

        end

        // [1T] АЛУ с непосредственным операндом
        16'b0011_xxxx_xxxx_xxxx, // CPI
        16'b0100_xxxx_xxxx_xxxx, // SBCI
        16'b0101_xxxx_xxxx_xxxx, // SUBI
        16'b0110_xxxx_xxxx_xxxx, // ORI
        16'b0111_xxxx_xxxx_xxxx: // ANDI
        begin

            pc      <= pcnext;
            op1     <= r[rdi];
            op2     <= K;
            reg_id  <= rdi;
            reg_w   <= (opcode[15:12] != 4'b0011); // CPI не писать
            sreg_w  <= 1'b1;

            case (opcode[15:12])
                4'b0011: alu <= 5;  // CPI
                4'b0100: alu <= 2;  // SBCI
                4'b0101: alu <= 6;  // SUBI
                4'b0110: alu <= 10; // ORI
                4'b0111: alu <= 8;  // ANDI
            endcase

        end

        // [1T] MOV Rd, Rr
        16'b0010_11xx_xxxx_xxxx:
        begin

            pc      <= pcnext;
            alu     <= 0;
            op2     <= r[rr];
            reg_id  <= rd;
            reg_w   <= 1'b1;

        end

        // [2T] RCALL k | ICALL | EICALL | CALL
        16'b1101_xxxx_xxxx_xxxx,
        16'b1001_0101_000x_1001,
        16'b1001_010x_xxxx_111x:
        case (tstate)

            // Запись PCL
            0: begin

                tstate  <= 1;
                address <= sp;
                wb      <= is_call ? pcnext2[7:0] : pcnext[7:0];
                w       <= 1'b1;
                sp_mth  <= `SPDEC;
                pc      <= pcnext;

            end

            // Запись PCH
            1: begin

                tstate  <= 0;
                address <= sp;
                wb      <= is_call ? pcnext[15:8] : pc[15:8];
                w       <= 1'b1;
                sp_mth  <= `SPDEC;

                // Метод вызова
                pc      <= is_call     ? ir : // CALL
                           opcode[14]  ? (pc + {{4{opcode[11]}}, opcode[11:0]}) : // RCALL
                                         Z;   // ICALL

            end

        endcase

        // [3T] RET / RETI
        16'b1001_0101_000x_1000:
        case (tstate)

            // Указатель адреса
            0: begin

                tstate  <= 1;
                address <= sp + 1;
                sp_mth  <= `SPINC;

            end

            // Чтение PCH
            1: begin

                tstate   <= 2;
                pc[15:8] <= din;
                address  <= sp + 1;
                sp_mth   <= `SPINC;
                kbit     <= din[0];
                aread    <= 1'b1;

            end

            // Чтение PCL
            2: begin

                tstate   <= 0;
                pc[ 7:0] <= din;
                alu      <= 11;
                op2      <= {sreg[7] | opcode[4], sreg[6:0]};
                sreg_w   <= 1;
                kbit     <= din[0];
                aread    <= 1'b1;

            end

        endcase

        // [2T] LD Rd, (X|Y|Z)+
        // [1T] ST Rd, (X|Y|Z)+
        16'b1001_00xx_xxxx_1100, // X
        16'b1001_00xx_xxxx_1101, // X+
        16'b1001_00xx_xxxx_1110, // -X
        16'b1001_00xx_xxxx_1001, // Y+
        16'b1001_00xx_xxxx_1010, // -Y
        16'b1001_00xx_xxxx_0001, // Z+
        16'b1001_00xx_xxxx_0010: // -Z
        case (tstate)

            // Установка указателя на память
            0: begin

                tstate <= opcode[9] ? 0 : 1;
                pc     <= pcnext;
                wb     <= r[rd];
                w      <= opcode[9];

                // Выбор адреса
                case (opcode[3:0])

                    4'b11_00: begin address <= X;  end
                    4'b11_01: begin address <= X;  wb2 <= Xp; reg_idw <= 2'b01; reg_ww <= 1; end
                    4'b11_10: begin address <= Xm; wb2 <= Xm; reg_idw <= 2'b01; reg_ww <= 1; end
                    4'b10_01: begin address <= Y;  wb2 <= Yp; reg_idw <= 2'b10; reg_ww <= 1; end
                    4'b10_10: begin address <= Ym; wb2 <= Ym; reg_idw <= 2'b10; reg_ww <= 1; end
                    4'b00_01: begin address <= Z;  wb2 <= Zp; reg_idw <= 2'b11; reg_ww <= 1; end
                    4'b00_10: begin address <= Zm; wb2 <= Zm; reg_idw <= 2'b11; reg_ww <= 1; end

                endcase

            end

            // Запись в регистр Rd (LD)
            1: begin

                tstate  <= 0;
                alu     <= 0;
                op2     <= din;
                reg_w   <= 1;
                reg_id  <= rd;
                kbit    <= din[0];
                aread   <= 1'b1;

            end

        endcase

        // [2T] LDD Y+q, Z+q
        // [1T] STD Y+q, Z+q
        16'b10x0_xxxx_xxxx_xxxx: // Y,Z
        case (tstate)

            // Установка указателя на память
            0: begin

                tstate  <= opcode[9] ? 0 : 1;
                pc      <= pcnext;
                address <= (opcode[3] ? Y : Z) + q;
                wb      <= r[rd];
                w       <= opcode[9];

            end

            // Запись в регистр Rd
            1: begin

                tstate  <= 0;
                alu     <= 0;
                op2     <= din;
                reg_w   <= 1;
                reg_id  <= rd;
                kbit    <= din[0];
                aread   <= 1'b1;

            end

        endcase

        // [1T] 0=COM, 1=NEG, 2=SWAP, 3=INC, 5=ASR, 6=LSR, 7=ROR, 10=DEC
        16'b1001_010x_xxxx_00xx,
        16'b1001_010x_xxxx_011x,
        16'b1001_010x_xxxx_0101,
        16'b1001_010x_xxxx_1010: begin

            pc      <= pcnext;
            op1     <= r[rd];   // Rd
            reg_w   <= 1;
            sreg_w  <= 1;
            reg_id  <= rd;

            case (opcode[3:0])

                4'b0000: alu <= 5'h0C; // COM
                4'b0001: alu <= 5'h0D; // NEG
                4'b0010: alu <= 5'h0E; // SWAP
                4'b0011: alu <= 5'h0F; // INC
                4'b0101: alu <= 5'h10; // ASR
                4'b0110: alu <= 5'h11; // LSR
                4'b0111: alu <= 5'h12; // ROR
                4'b1010: alu <= 5'h13; // DEC

            endcase

        end

        // [2T] MOVW Rd, Rr
        16'b0000_0001_xxxx_xxxx:
        case (tstate)

            // LO регистр
            0: begin

                tstate  <= 1;
                pc      <= pcnext;
                alu     <= 0;
                op2     <= r[ {opcode[3:0], 1'b0} ];
                reg_id  <=    {opcode[7:4], 1'b0};
                reg_w   <= 1;

            end

            // HI регистр
            1: begin

                tstate  <= 0;
                alu     <= 0;
                op2     <= r[ {opcode[3:0], 1'b1} ];
                reg_id  <=    {opcode[7:4], 1'b1};
                reg_w   <= 1;

            end

        endcase

        // [1T] <ADIW|SBIW> Rd, K
        16'b1001_0110_xxxx_xxxx, // ADIW
        16'b1001_0111_xxxx_xxxx: // SBIW
        begin

            pc      <= pcnext;
            alu     <= opcode[8] ? 5'h15 : 5'h14;

            case (opcode[5:4])
                0: op1w <= {r[25], r[24]};
                1: op1w <= {r[27], r[26]};
                2: op1w <= {r[29], r[28]};
                3: op1w <= {r[31], r[30]};
            endcase

            op2     <= {opcode[7:6], opcode[3:0]};
            reg_idw <= opcode[5:4];
            reg_ww  <= 1;
            reg_ws  <= 1;
            sreg_w  <= 1;

        end

        // [2T] LPM Rd, Z
        16'b1001_0101_110x_1000, // LPM R0, Z  | ELPM R0, Z
        16'b1001_000x_xxxx_01x0, // LPM Rd, Z  | ELPM Rd, Z
        16'b1001_000x_xxxx_01x1: // LPM Rd, Z+ | ELPM Rd, Z+
        case (tstate)

            // Считывание байта
            0: begin

                tstate  <= 1;
                pclatch <= pcnext;

                // Если ELPM, то записать в старший бит rampz[0]
                if (opcode[1] || (opcode[10] && opcode[4]))
                     pc <= {rampz[0], Z[15:1]}; // ELPM
                else pc <= Z[15:1];             // LPM

            end

            // Запись
            1: begin

                tstate  <= 0;
                pc      <= pclatch;
                alu     <= 0;       // LDI
                reg_idw <= 2'b11;   // Z
                reg_ww  <= (!opcode[10] & opcode[0]); // Z+
                wb2     <= Zp;
                reg_w   <= 1;
                reg_id  <= opcode[10] ? 0 : rd;         // R0, Rd
                op2     <= Z[0] ? ir[15:8] : ir[7:0];   // Hi, Lo

            end


        endcase

        // [2T] IN  Rd, A
        // [1T] OUT A, Rd
        16'b1011_xxxx_xxxx_xxxx:
        case (tstate)

            // Установка адреса
            0: begin

                tstate  <= opcode[11] ? 0 : 1;
                pc      <= pcnext;
                wb      <= r[rd];
                w       <= opcode[11];
                address <= {opcode[10:9], opcode[3:0]} + 16'h20;

            end

            // Запись регистра
            1: begin

                tstate  <= 0;
                alu     <= 0;
                op2     <= din;
                reg_id  <= rd;
                reg_w   <= 1;
                kbit    <= din[0];
                aread   <= 1'b1;

            end

        endcase

        // [1T] SBR[C,S] Rd, b
        // [1T] SBRS Rd, b
        16'b1111_11xx_xxxx_0xxx: begin

            pc <= pcnext;
            if (r[rd][ opcode[2:0] ] == opcode[9])
                skip_instr <= 1;

        end

        // [2T] SBI[C,S] A, b
        // [2T] SBIS A, b
        16'b1001_10x1_xxxx_xxxx: // C=0,S=1
        casex (tstate)

            // Запрос чтения
            0: begin

                tstate  <= 1;
                address <= opcode[7:3] + 16'h20;

            end

            // Вычисление бита
            1: begin

                tstate  <= 0;
                kbit    <= din[0];
                aread   <= 1'b1;
                pc      <= pcnext;

                if (din[ opcode[2:0] ] == opcode[9])
                    skip_instr <= 1;

            end

        endcase

        // [1T] CPSE Rd, Rr
        16'b0001_00xx_xxxx_xxxx: begin

            if (r[rd] == r[rr]) skip_instr <= 1;
            pc <= pcnext;

        end

        // [3T] LDS Rd, Mem
        // [2T] STS Mem, Rd
        16'b1001_00xx_xxxx_0000: // 0=lds, 1=sts
        case (tstate)

            // К следующему коду
            0: begin

                tstate  <= 1;
                pc      <= pcnext;

            end

            // Запись в память или выбор регистра
            1: begin

                tstate  <= opcode[9] ? 0 : 2; // 1=STS, 0=LDS
                pc      <= pcnext;
                reg_id  <= rd;
                address <= ir;
                wb      <= r[rd];
                w       <= opcode[9];

            end

            // Запись в регистр
            2: begin

                tstate  <= 0;
                alu     <= 0;
                reg_w   <= 1;
                op2     <= din;
                kbit    <= din[0];
                aread   <= 1'b1;

            end

        endcase

        // [1T] BCLR, BSET
        16'b1001_0100_xxxx_1000: begin

            case (opcode[6:4])

                0: op2 <= {sreg[7:1], !opcode[7]};
                1: op2 <= {sreg[7:2], !opcode[7], sreg[0]};
                2: op2 <= {sreg[7:3], !opcode[7], sreg[1:0]};
                3: op2 <= {sreg[7:4], !opcode[7], sreg[2:0]};
                4: op2 <= {sreg[7:5], !opcode[7], sreg[3:0]};
                5: op2 <= {sreg[7:6], !opcode[7], sreg[4:0]};
                6: op2 <= {sreg[7],   !opcode[7], sreg[5:0]};
                7: op2 <= {!opcode[7],            sreg[6:0]};

            endcase

            alu     <= 11;
            sreg_w  <= 1;
            pc      <= pcnext;

        end

        // [1T] PUSH Rd
        16'b1001_001x_xxxx_1111: begin

            pc      <= pcnext;
            wb      <= r[rd];
            w       <= 1;
            address <= sp;
            sp_mth  <= `SPDEC;

        end

        // [2T] POP Rd
        16'b1001_000x_xxxx_1111:
        case (tstate)

            // Указатель адреса
            0: begin

                tstate  <= 1;
                pc      <= pcnext;
                address <= sp + 1;
                sp_mth  <= `SPINC;

            end

            // Запись в регистр
            1: begin

                tstate  <= 0;
                alu     <= 0;
                op2     <= din;
                reg_id  <= rd;
                reg_w   <= 1;
                kbit    <= din[0];
                aread   <= 1'b1;

            end

        endcase

        // [1T] BST Rd, b
        16'b1111_101x_xxxx_0xxx: begin

            pc      <= pcnext;
            alu     <= 11;
            op2     <= {sreg[7], r[rd][ opcode[2:0] ], sreg[5:0]};
            sreg_w  <= 1;

        end

        // [1T] BLD Rd, b
        16'b1111_100x_xxxx_0xxx: begin

            pc      <= pcnext;
            alu     <= 22;  // BLD
            op1     <= r[rd];
            op2     <= opcode[2:0];
            reg_id  <= rd;
            reg_w   <= 1;

        end

        // [2T] CBI / SBI A, b
        16'b1001_10x0_xxxx_xxxx:
        case (tstate)

            // Чтение из порта
            0: begin

                tstate  <= 1;
                pc      <= pcnext;
                address <= opcode[7:3] + 16'h20;

            end

            // Запись в порт
            1: begin

                tstate  <= 0;
                kbit    <= din[0];
                aread   <= 1'b1;

                case (opcode[2:0])
                    0: wb <= {din[7:1], opcode[9]};
                    1: wb <= {din[7:2], opcode[9], din[0]};
                    2: wb <= {din[7:3], opcode[9], din[1:0]};
                    3: wb <= {din[7:4], opcode[9], din[2:0]};
                    4: wb <= {din[7:5], opcode[9], din[3:0]};
                    5: wb <= {din[7:6], opcode[9], din[4:0]};
                    6: wb <= {din[  7], opcode[9], din[5:0]};
                    7: wb <= {          opcode[9], din[6:0]};
                endcase
                w <= 1;

            end

        endcase

    endcase

end
end

// Запись в регистры
always @(negedge clock) begin

    // Блокировка процессора на время I/O
    if (!locked) begin

        if (reg_w) r[ reg_id ] <= alu_res;
        if (sreg_w) sreg <= alu_sreg;

        // Икремент или декремент
        case (sp_mth)

            `SPDEC: sp <= sp - 1;
            `SPINC: sp <= sp + 1;

        endcase

        // Автоинкремент или декремент X,Y,Z
        if (reg_ww) begin

            case (reg_idw)

                0: {r[25], r[24]} <= reg_ws ? resw : wb2; // W
                1: {r[27], r[26]} <= reg_ws ? resw : wb2; // X
                2: {r[29], r[28]} <= reg_ws ? resw : wb2; // Y
                3: {r[31], r[30]} <= reg_ws ? resw : wb2; // Z

            endcase

        end

        // Запись в порты или регистры
        if (w) begin

            case (address)

                // Управление памятью, курсором, SPI
                16'h0020: bank     <= wb; // memory.bank (4kb) $F000
                16'h0024: cursor_x <= wb; // text.cursor.x
                16'h0025: cursor_y <= wb; // text.cursor.y
                16'h0028: spi_out  <= wb; // spi.data

                // Запуск триггера активации команды SPI
                16'h0029: begin

                    spi_cmd  <= wb[1:0];
                    spi_sent <= 1'b1;
                    locked   <= 1'b1;

                end

                16'h002D: vmode     <= wb; // Видеорежим

                // SDRAM
                16'h0030: sdram_address[ 7:0]  <= wb;
                16'h0031: sdram_address[15:8]  <= wb;
                16'h0032: sdram_address[23:16] <= wb;
                16'h0033: sdram_address[31:24] <= wb;
                16'h0034: sdram_o_data         <= wb;
                16'h0035: sdram_control        <= wb;

                // Системные регистры
                16'h005B: rampz     <= wb; // Верхняя память ROM
                16'h005D: sp[ 7:0]  <= wb; // SPL
                16'h005E: sp[15:8]  <= wb; // SPH
                16'h005F: sreg      <= wb; // SREG

                // Запись в регистры как в память
                default:

                    if (address < 16'h20) r[ address[4:0] ] <= wb;

            endcase

        end

        // Тест на чтение из порта
        if (aread) begin

            // Сбросить бит 0 в порту 23h. Реагировать только если бит был реально прочитан!
            if (address == 16'h0023 && kbit) kb_trigger <= kb_trigger ^ kb_hit;

        end

    end

    // -----------------------------------------------------------------
    // Разблокировка процессора при BUSY=0
    if (spi_proc && spi_st[0] == 1'b0) begin
        locked   <= 0;
        spi_proc <= 0;
    end

    // Сброс SENT, если устройство SPI начало работу
    if (spi_sent && spi_st[0]) begin
        spi_sent <= 1'b0;
        spi_proc <= 1'b1;
    end
    // -----------------------------------------------------------------

end

endmodule
