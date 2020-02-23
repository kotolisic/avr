/*
 * Модуль памяти 64Мб для DE0-CV
 * Реальная частота работы чтения-записи без конвейера 2.5 Мгц
 */

module sdram
(
    // Тактовая частота 100 МГц (SDRAM)
    input  wire         clock_100_mhz,
    input  wire         clock_25_mhz,

    // Управление
    input  wire [25:0]  i_address,      // 64 МБ памяти
    input  wire         i_we,           // Признак записи в память
    input  wire [ 7:0]  i_data,         // Данные для записи (8 бит)
    output reg  [ 7:0]  o_data,         // Прочитанные данные
    output reg          o_ready,        // Готовность данных (=1 Готово)

    // Физический интерфейс DRAM
    output wire         dram_clk,       // Тактовая частота памяти
    output reg  [ 1:0]  dram_ba,        // 4 банка
    output wire [12:0]  dram_addr,      // Максимальный адрес 2^13=8192
    inout  wire [15:0]  dram_dq,        // Ввод-вывод
    output wire         dram_cas,       // CAS
    output wire         dram_ras,       // RAS
    output wire         dram_we,        // WE
    output reg          dram_ldqm,      // Маска для младшего байта
    output reg          dram_udqm       // Маска для старшего байта
);

// Command modes                 RCW
// ---------------------------------------------------------------------
// Команды
localparam cmd_loadmode     = 3'b000;
localparam cmd_refresh      = 3'b001;
localparam cmd_precharge    = 3'b010;
localparam cmd_activate     = 3'b011;
localparam cmd_write        = 3'b100;
localparam cmd_read         = 3'b101;
localparam cmd_burst_term   = 3'b110;
localparam cmd_nop          = 3'b111;

// Режимы работы машины состояний
localparam state_idle      = 0;
localparam state_rw        = 1;
localparam state_precharge = 2;

// Перезарядка (2 раза в сек пока что)
localparam precharge_timeout = 50000000;

`ifdef ICARUS
localparam init_time = 0;
`else
localparam init_time = 5000;
`endif

initial begin

    o_ready   = 0;
    o_data    = 8'hFF;
    dram_ba   = 0;
    dram_ldqm = 1;
    dram_udqm = 1;

end

// Связь с физическим интерфейсом памяти
// ---------------------------------------------------------------------

// Направление данных (in, out), если dram_we=0, то запись; иначе чтение
assign dram_dq   =  dram_we ? 16'hZZZZ : {i_data, i_data};
assign dram_clk  =  clock_100_mhz;

// Адрес и команда
assign {dram_addr                  } = chipinit ? dram_init    : address;
assign {dram_ras, dram_cas, dram_we} = chipinit ? command_init : command;

// Команды для памяти, регистры, текущее состояние
// ---------------------------------------------------------------------
reg             chipinit        = 1;
reg  [14:0]     icounter        = 0;
reg  [2:0]      command         = cmd_nop;
reg  [2:0]      command_init    = cmd_nop;
reg  [12:0]     dram_init       = 12'b1_00000_00000;
reg  [ 1:0]     current_state   = state_idle;
reg  [12:0]     address         = 0;
reg  [24:0]     current_addr    = 0;
reg  [ 3:0]     cursor          = 0;
reg  [25:0]     w_address       = 0;
reg             w_request       = 0;
reg             first_enabled   = 1;
reg  [25:0]     precharge_time  = 0;

// Инициализация чипа памяти
// Параметры: BurstFull, Sequential, CASLatency=2
// ---------------------------------------------------------------------
always @(posedge clock_25_mhz)
if (chipinit) begin

    case (icounter)

        init_time + 1:  begin command_init <= cmd_precharge; end
        init_time + 4:  begin command_init <= cmd_refresh; end
        init_time + 18: begin command_init <= cmd_loadmode; dram_init[9:0] <= 10'b0_00_010_0_111; end
        init_time + 21: begin chipinit     <= 0; end
        default:        begin command_init <= cmd_nop; end

    endcase

    icounter <= icounter + 1;

end

// Основной обработчик (видеокарта, ввод-вывод в память)
// Работает после инициализации памяти
// ---------------------------------------------------------------------
always @(posedge clock_100_mhz)
if (~chipinit) begin

    case (current_state)

        // Режим ожидания чтения, записи, или новой строки
        // -----------------------------------------
        state_idle: begin

            // Время ожидания для строк вышло
            if (precharge_time > precharge_timeout) begin

                o_ready        <= 1'b0;
                precharge_time <= 0;
                cursor         <= 0;
                current_state  <= state_precharge;
                current_addr   <= 0;

            end

            // 1. Обнаружено изменение адреса
            // 2. Первое включение памяти
            // 3. Обнаружена запись, данные поменялись
            // ---------------------------------------------------------
            else if (first_enabled || i_address != w_address || (i_we && o_data != i_data)) begin

                o_ready       <= 1'b0;
                first_enabled <= 1'b0;
                cursor        <= 0;

                // Запись адреса и типа запроса
                w_request     <= i_we;
                w_address     <= i_address;
                current_addr  <= i_address[25:1];
                current_state <= state_rw;

                // Активация строки
                command       <= cmd_activate;
                address       <= i_address[23:11];
                dram_ba       <= i_address[25:24];

                if (i_we) begin

                    dram_udqm <= ~i_address[0]; // bit=1, значит, HIGH
                    dram_ldqm <=  i_address[0]; // bit=0, значит, LOW

                end else begin

                    dram_udqm <= 1'b0;
                    dram_ldqm <= 1'b0;

                end

                // Адрес потребуется для чтения или записи
                if (i_we) o_data <= i_data;

                // Учитывать это время для перезарядки
                precharge_time <= precharge_time + 10;

            end

            // Готовность
            else begin

                o_ready        <= 1'b1;
                precharge_time <= precharge_time + 1;

            end

        end

        // 7 Запись, 10 Чтение (такты)
        // -----------------------------------------
        state_rw: case (cursor)

            // Ожидание активации строки
            0, 1: begin command <= cmd_nop; cursor <= cursor + 1; end

            // Запись или чтение
            2: begin

                cursor  <= 3;
                command <= w_request ? cmd_write : cmd_read;
                address <= {1'b1, current_addr[9:0]};

            end

            // Для записи слова требуется BURST Terminate
            3: if (w_request)
            begin cursor <= 6; command <= cmd_burst_term;  end
            else  cursor <= 4;

            // Для корректного чтения требуется 3 такта, чтобы успел сигнал
            4, 5: cursor <= cursor + 1;

            // Перезарядка банка, закрытие строки
            6: begin

                cursor      <= 7;
                command     <= cmd_precharge;
                address[10] <= 1'b1;

                if (w_request == 1'b0)
                    o_data <= (w_address[0] ? dram_dq[15:8] : dram_dq[7:0]);

            end

            // Переход к IDLE
            7: begin

                cursor        <= 0;
                command       <= cmd_nop;
                dram_udqm     <= 1'b1;
                dram_ldqm     <= 1'b1;
                current_state <= state_idle;

            end

        endcase

        // Перезарядка всех банков
        state_precharge: case (cursor)

            // Активация
            0: begin

                command <= cmd_activate;
                address <= current_addr[12:0];
                dram_ba <= current_addr[14:13]; 
                cursor  <= 1;

            end

            // Ожидание Activate
            1, 2: begin cursor <= cursor + 1; command <= cmd_nop; end

            // Перезарядка
            3: begin

                cursor      <= cursor + 1;
                command     <= cmd_precharge;
                address[10] <= 1'b1;

            end

            // Ожидание
            4: begin

                cursor  <= 0;
                command <= cmd_nop;

                // Выход из цикла перезаряда
                if (current_addr[14:0] == 15'h7FFF)
                    current_state <= state_idle;

                current_addr[14:0] <= current_addr[14:0] + 1;

            end


        endcase

    endcase

end

endmodule
