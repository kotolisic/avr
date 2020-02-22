/*
 * Модуль памяти 64Мб для DE0-CV
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

localparam request_none    = 0;
localparam request_write   = 1;
localparam request_read    = 2;

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
reg  [ 3:0]     current_state   = state_idle;
reg  [12:0]     address         = 0;
reg  [24:0]     current_addr    = 0;
reg  [ 3:0]     cursor          = 0;
reg  [25:0]     w_address       = 0;
reg  [ 1:0]     rw_request      = 0;
reg             first_enabled   = 1;

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

            // PRECHARGE ROW

            // 1. Обнаружено изменение адреса
            // 2. Первое включение памяти
            // 3. Обнаружена запись, данные поменялись
            // ---------------------------------------------------------
            if (first_enabled || i_address != w_address || (i_we && o_data != i_data)) begin

                o_ready       <= 1'b0;
                first_enabled <= 1'b0;
                cursor        <= 0;

                // Запись адреса и типа запроса
                w_address     <= i_address;
                current_addr  <= i_address[25:1];
                current_state <= state_rw;
                rw_request    <= i_we ? request_write : request_read;

                // Активация строки
                command       <= cmd_activate;
                address       <= i_address[23:11];
                dram_ba       <= i_address[25:24];
                dram_udqm     <= ~i_address[0]; // bit=1, значит, HIGH
                dram_ldqm     <=  i_address[0]; // bit=0, значит, LOW

                // Адрес потребуется для чтения или записи
                if (i_we) o_data <= i_data;

            end

            // Готовность
            else begin o_ready <= 1'b1; end

        end

        // Запись в память
        // -----------------------------------------
        state_rw: case (cursor)

            // Ожидание активации строки
            0, 1: begin command <= cmd_nop; cursor <= cursor + 1; end

            // Запись/Чтение
            2: begin

                cursor  <= 3;
                command <= rw_request == request_write ? cmd_write : cmd_read;
                address <= {1'b1, current_addr[9:0]};

            end

            // При записи использовать NOP
            3, 4: begin

                if (rw_request == request_write) command <= cmd_nop;
                // else address[9:0] <= address[9:0] + 1;

                cursor <= cursor + 1;

            end

            // Перезарядка банка, закрытие строки
            5: begin

                cursor      <= 6;
                command     <= cmd_precharge;
                address[10] <= 1'b1;
                dram_udqm   <= 1'b1;
                dram_ldqm   <= 1'b1;

                if (rw_request == request_read)
                    o_data <= w_address[0] ? dram_dq[15:8] : dram_dq[7:0];

            end

            // Переход к IDLE
            6: begin

                cursor        <= 0;
                command       <= cmd_nop;
                current_state <= state_idle;

            end

        endcase

    endcase

end

endmodule
