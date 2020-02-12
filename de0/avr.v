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
// Центральный процессорный блок
// ---------------------------------------------------------------------

cpu UnitAVRCPU
(
    clklo & locked,
    pc, ir,
    mem_id,
    mem,
    mem_wb,
    mem_w,
    bank,
    vmode,
    // KEYB
    kb_ch,
    kb_tr,
    kb_hit,
    // CURSOR
    cursor_x,
    cursor_y,
    mouse_x,
    mouse_y,
    mouse_cmd,
    // SPI
    spi_sent,
    spi_cmd,
    spi_din,
    spi_out,
    spi_st
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
