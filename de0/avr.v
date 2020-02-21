`timescale 10ns / 1ns

module avr;

// ---------------------------------------------------------------------
// Симулятор Verilog Icarus
// ---------------------------------------------------------------------

reg clock  = 0;
reg clk50  = 0;
reg clklo  = 0;
reg locked = 0;

always #0.5 clock = ~clock;
always #1.0 clk50 = ~clk50;
always #1.5 clklo = ~clklo;

initial begin clock = 0; clk50 = 1; clklo = 0; #3.0 locked = 1; #2000 $finish; end
initial begin $dumpfile("avr.vcd"); $dumpvars(0, avr); end

// ---------------------------------------------------------------------
// Интерфейс для работы с памятью (чтение и запись)
// ---------------------------------------------------------------------

reg [15:0] flash[65536]; // 128k
reg [ 7:0] sram[65536];  // 64kb

initial $readmemh("avr.hex", flash, 16'h0000);
initial $readmemh("sram.hex", sram, 16'h0000);

wire [15:0] pc;     reg [15:0] _ir;  reg [15:0] ir;
wire [15:0] mem_id; reg [ 7:0] _mem; reg [ 7:0] mem;
wire [ 7:0] mem_wb;
wire        mem_w;
wire [ 7:0] bank;
wire [ 1:0] vmode;

wire [ 7:0] kb_ch;
wire        kb_tr;
wire        kb_hit;
wire [ 7:0] cursor_x;
wire [ 7:0] cursor_y;

// SPI
wire        spi_sent;
wire [ 1:0] spi_cmd;
wire [ 7:0] spi_din;
wire [ 7:0] spi_out;
wire [ 1:0] spi_st;

// Mouse
wire [ 8:0] mouse_x;
wire [ 7:0] mouse_y;
wire [ 1:0] mouse_cmd;

always @(posedge clock) begin

    ir  <= _ir;  _ir  <= flash[pc];
    mem <= _mem; _mem <= sram[mem_id];

    if (mem_w) sram[mem_id] <= mem_wb;

end

// ---------------------------------------------------------------------
// Интерфейс SDRAM
// ---------------------------------------------------------------------

wire [31:0] sdram_address;
wire [ 7:0] sdram_i_data;
wire [ 7:0] sdram_o_data;
wire [ 7:0] sdram_control;

// ---------------------------------------------------------------------
// Центральный процессорный блок
// ---------------------------------------------------------------------

cpu UnitAVRCPU
(
    .clock      (clklo & locked),

    // Программная память
    .pc         (pc),          // Программный счетчик
    .ir         (ir),          // Инструкция из памяти

    // Оперативная память
    .address    (mem_id),     // Указатель на память RAM (sram)
    .din_raw    (mem),     // memory[ address ]
    .wb         (mem_wb),          // Запись в память по address
    .w          (mem_w),           // Разрешение записи в память
    .bank       (bank),        // Банк памяти
    .vmode      (vmode),       // Видеорежим

    // Ввод-вывод
    .kb_ch      (kb_ch),       // Клавиатура
    .kb_tr      (kb_tr),       // Триггер внешний
    .kb_hit     (kb_hit),

    // Положение курсора
    .cursor_x   (cursor_x),
    .cursor_y   (cursor_y),

    // Мышь
    .mouse_x    (mouse_x),
    .mouse_y    (mouse_y),
    .mouse_cmd  (mouse_cmd),

    // SPI
    .spi_sent   (spi_sent),     // =1 Сообщение отослано на spi
    .spi_cmd    (spi_cmd),     // Команда
    .spi_din    (spi_din),     // Принятое сообщение
    .spi_out    (spi_out),     // Сообщение на отправку
    .spi_st     (spi_st),      // bit 1: timeout (1); bit 0: busy

    // SDRAM
    .sdram_address  (sdram_address),
    .sdram_i_data   (sdram_i_data),
    .sdram_o_data   (sdram_o_data),
    .sdram_control  (sdram_control)    
);

// ---------------------------------------------------------------------
// SPI
// ---------------------------------------------------------------------
wire spi_cs;
wire spi_sclk;
wire spi_miso = 1'b1;
wire spi_mosi;

spi UnitSPI(

    clk50,
    
    // SPI
    spi_cs,
    spi_sclk,
    spi_miso,
    spi_mosi,

    // Интерфейс
    spi_sent,
    spi_cmd,
    spi_din,
    spi_out,
    spi_st
);

endmodule
