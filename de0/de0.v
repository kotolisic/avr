module de0(

      /* Reset */
      input              RESET_N,

      /* Clocks */
      input              CLOCK_50,
      input              CLOCK2_50,
      input              CLOCK3_50,
      inout              CLOCK4_50,

      /* DRAM */
      output             DRAM_CKE,
      output             DRAM_CLK,
      output      [1:0]  DRAM_BA,
      output      [12:0] DRAM_ADDR,
      inout       [15:0] DRAM_DQ,
      output             DRAM_CAS_N,
      output             DRAM_RAS_N,
      output             DRAM_WE_N,
      output             DRAM_CS_N,
      output             DRAM_LDQM,
      output             DRAM_UDQM,

      /* GPIO */
      inout       [35:0] GPIO_0,
      inout       [35:0] GPIO_1,

      /* 7-Segment LED */
      output      [6:0]  HEX0,
      output      [6:0]  HEX1,
      output      [6:0]  HEX2,
      output      [6:0]  HEX3,
      output      [6:0]  HEX4,
      output      [6:0]  HEX5,

      /* Keys */
      input       [3:0]  KEY,

      /* LED */
      output      [9:0]  LEDR,

      /* PS/2 */
      inout              PS2_CLK,
      inout              PS2_DAT,
      inout              PS2_CLK2,
      inout              PS2_DAT2,

      /* SD-Card */
      output             SD_CLK,
      inout              SD_CMD,
      inout       [3:0]  SD_DATA,

      /* Switch */
      input       [9:0]  SW,

      /* VGA */
      output      [3:0]  VGA_R,
      output      [3:0]  VGA_G,
      output      [3:0]  VGA_B,
      output             VGA_HS,
      output             VGA_VS
);

// MISO: Input Port
assign SD_DATA[0] = 1'bZ;

// SDRAM Enable
assign DRAM_CKE  = 1; // ChipEnable
assign DRAM_CS_N = 0; // ChipSelect

// Z-state
assign DRAM_DQ = 16'hzzzz;
assign GPIO_0  = 36'hzzzzzzzz;
assign GPIO_1  = 36'hzzzzzzzz;

// LED OFF
assign HEX0 = 7'b1111111;
assign HEX1 = 7'b1111111;
assign HEX2 = 7'b1111111;
assign HEX3 = 7'b1111111;
assign HEX4 = 7'b1111111;
assign HEX5 = 7'b1111111;

pll u0
(
    .clkin (CLOCK_50),
    .m25   (clk25),
    .m12   (clk12),
    .m50   (clk50),
    .m100  (clk),
    .locked (locked)
);

// ---------------------------------------------------------------------
// Видеоадаптер
// ---------------------------------------------------------------------

wire [11:0] font_addr;
wire [ 7:0] font_data;
wire [11:0] char_addr;
wire [ 7:0] char_data;
wire [ 7:0] cursor_x;
wire [ 7:0] cursor_y;
wire [ 1:0] vmode;
wire [14:0] gm_address;
wire [ 7:0] gm_data;

// Видеоадаптер
vga UnitVGATextDisplay
(
    .CLK25  (clk25),
    .vmode  (vmode),

    // Физический интерфейс
    .VGA_R  (VGA_R),
    .VGA_G  (VGA_G),
    .VGA_B  (VGA_B),
    .VGA_HS (VGA_HS),
    .VGA_VS (VGA_VS),

    // Адреса таблиц шрифтов
    .font_addr  (font_addr),
    .font_data  (font_data),
    .char_addr  (char_addr),
    .char_data  (char_data),

    // Указатель курсора
    .cursor_x   (cursor_x),
    .cursor_y   (cursor_y),

    // Графический режим 320x200x4
    .gm_address (gm_address),
    .gm_data    (gm_data)
);

// Знакогенератор 4k
fontrom UnitFontRom
(
    // Чтение из памяти
    .clock     (clk),
    .address_a (font_addr),
    .q_a       (font_data),

    // Запись в память ROM
    .address_b  (address[11:0]),
    .data_b     (wb),
    .wren_b     (w & w_font),
    .q_b        (din_font)
);

// ---------------------------------------------------------------------
// Процессор
// ---------------------------------------------------------------------

// Разрешение на запись в память
wire m_bank   = (address >= 16'hF000);
wire w_sram   = (bank == 0) || (address < 16'hF000);
wire w_font   = m_bank && (bank == 1);
wire w_extram = m_bank && (bank[7:4] == 4'h1);
wire w_gram   = m_bank && (bank[7:3] == 5'b00001);

wire [15:0] pc;
wire [15:0] address;
wire [15:0] ir;

// Роутинг памяти
wire [ 7:0] din_sram;
wire [ 7:0] din_font;
wire [ 7:0] din_extram;
wire [ 7:0] din_gram;

// Откуда получаются данные?
wire [ 7:0] din = w_sram ? din_sram :
                  w_font ? din_font :
                  w_gram ? din_gram :
                           din_extram;

wire [ 7:0] wb;
wire        w;
wire [ 7:0] bank;

// 128Kb памяти программ
flash UnitFlashMemory
(
    .clock      (clk),
    .address_a  (pc),
    .q_a        (ir),
);

// 64 Kb общей памяти
sram UnitSRAM
(
    .clock      (clk),
    .address_a  (address),
    .q_a        (din_sram),
    .data_a     (wb),
    .wren_a     (w & w_sram),

    // $F000 - $FFFF Диапазон в банке 0 (видеоданные)
    .address_b  ({4'b1111, char_addr}),
    .q_b        (char_data)
);

// Расширенная память 64K [и видеоадаптер 640x400x2, 320x200x8]
extram UnitExtendRAM64
(
    .clock      (clk),
    .address_a  ({bank[3:0], address[11:0]}),
    .q_a        (din_extram),
    .data_a     (wb),
    .wren_a     (w & w_extram),

    // Для видеоадаптера
    // .address_b(),
    // .q_b      ()
);

// Память видеоадаптера, графический 32к
gram UnitGVideoRAM32
(
    .clock      (clk),
    .address_a  ({bank[2:0], address[11:0]}),
    .q_a        (din_gram),
    .data_a     (wb),
    .wren_a     (w & w_gram),

    // Запрос видеоадаптера
    .address_b  (gm_address),
    .q_b        (gm_data),
);

cpu UnitAVRCPU
(
    .clock      (clk25 & locked),
    .pc         (pc),
    .ir         (ir),
    .address    (address),
    .din_raw    (din),
    .wb         (wb),
    .w          (w),
    .bank       (bank),
    .vmode      (vmode),

    // Keyboard
    .kb_ch      (kb_ch),
    .kb_tr      (kb_tr),
    .kb_hit     (kb_hit),

    // Cursor
    .cursor_x   (cursor_x),
    .cursor_y   (cursor_y),
    .mouse_x    (mouse_x),
    .mouse_y    (mouse_y),
    .mouse_cmd  (mouse_cmd),

    // SPI
    .spi_sent   (spi_sent),
    .spi_cmd    (spi_cmd),
    .spi_din    (spi_din),
    .spi_out    (spi_out),
    .spi_st     (spi_st),

    // SDRAM
    .sdram_address  (sdram_address),
    .sdram_i_data   (sdram_i_data),
    .sdram_o_data   (sdram_o_data),
    .sdram_status   (sdram_status),     // bit 0: Ready
    .sdram_control  (sdram_control)     // bit 0: WE
);

// Контроллер клавиатуры PS/2
// ---------------------------------------------------------------------

reg         kbd_reset        = 1'b0;
reg [7:0]   ps2_command      = 1'b0;
reg         ps2_command_send = 1'b0;
wire        ps2_command_was_sent;
wire        ps2_error_communication_timed_out;
wire [7:0]  ps2_data;
wire        ps2_data_clk;
reg         kb_unpressed   = 1'b0;
wire [7:0]  keyb_xt;
reg         kb_tr = 1'b0;
reg  [7:0]  kb_ch = 8'h00;

PS2_Controller Keyboard
(
	/* Вход */
    .CLOCK_50       (clk50),
	.reset          (kbd_reset),
	.the_command    (ps2_command),
	.send_command   (ps2_command_send),

	/* Ввод-вывод */
	.PS2_CLK        (PS2_CLK),
 	.PS2_DAT        (PS2_DAT),

	/* Статус команды */
	.command_was_sent               (ps2_command_was_sent),
	.error_communication_timed_out  (ps2_error_communication_timed_out),

    /* Выход полученных */
	.received_data      (ps2_data),
	.received_data_en   (ps2_data_clk)
);

// Преобразование AT-кода
ps2_at2xt UnitPS2XT
(
    .keyb_at    (ps2_data),
    .keyb_xt    (keyb_xt),
);

// Новые данные присутствуют
always @(posedge clk50) begin

    if (ps2_data_clk) begin

        // Признак отпущенной клавиши
        if (ps2_data == 8'hF0) begin
            kb_unpressed <= 1'b1;

        end else begin

            // Если старший бит равен 1, то это специальный код
            kb_ch <= keyb_xt[7] ? keyb_xt[7:0] : {kb_unpressed, keyb_xt[6:0]};
            kb_tr <= (kb_tr ^ kb_hit ^ 1'b1); // kb_hit установить в 1
            kb_unpressed <= 1'b0;

        end

    end

end

// Контроллер SPI
// ---------------------------------------------------------------------

wire        spi_sent;
wire [1:0]  spi_cmd;
wire [1:0]  spi_st;
wire [7:0]  spi_din;
wire [7:0]  spi_out;

spi UnitSPI(

    // 50 Mhz
    .clock50    (clk50),

    // Физический интерфейс
    .spi_cs     (SD_DATA[3]),  // Выбор чипа
    .spi_sclk   (SD_CLK),      // Тактовая частота
    .spi_miso   (SD_DATA[0]),  // Входящие данные
    .spi_mosi   (SD_CMD),      // Исходящие

    // Интерфейс
    .spi_sent   (spi_sent),    // =1 Сообщение отослано на spi
    .spi_cmd    (spi_cmd),     // Команда
    .spi_din    (spi_din),     // Принятое сообщение
    .spi_out    (spi_out),     // Сообщение на отправку
    .spi_st     (spi_st)       // bit 0: timeout (1); bit 1: chip select 0/1
);

// ---------------------------------------------------------------------
// Мышь
// ---------------------------------------------------------------------

reg [8:0] mouse_x = 0;
reg [7:0] mouse_y = 0;
reg [1:0] mouse_cmd = 0;

reg [18:0] mouse_cnt;

always @(posedge clk50) begin

    if (mouse_cnt == 0) begin

        if (KEY[3] == 0 && mouse_x > 0)   mouse_x <= mouse_x - 1;
        if (KEY[0] == 0 && mouse_x < 319) mouse_x <= mouse_x + 1;
        if (KEY[2] == 0 && mouse_y > 0)   mouse_y <= mouse_y - 1;
        if (KEY[1] == 0 && mouse_y < 200) mouse_y <= mouse_y + 1;

        mouse_cmd <= ~RESET_N;

    end

    mouse_cnt <= mouse_cnt + 1;

end

// ---------------------------------------------------------------------
// SDRAM
// ---------------------------------------------------------------------

wire [31:0] sdram_address;
wire [ 7:0] sdram_i_data;
wire [ 7:0] sdram_o_data;
wire [ 7:0] sdram_control;
wire [ 7:0] sdram_status = {6'h0, o_ready, sdram_control[0]};
wire        o_ready;

sdram UnitSDRAM
(
    // Тактовая частота 100 МГц (SDRAM)
    .clock_100_mhz  (clk),
    .clock_25_mhz   (clk25),

    // Управление
    .i_address      (sdram_address[25:0]),  // 64 МБ памяти
    .i_we           (sdram_control[0]),     // Признак записи в память
    .i_data         (sdram_o_data),         // Данные для записи (8 бит)
    .o_data         (sdram_i_data),         // Прочитанные данные
    .o_ready        (o_ready),              // Готовность данных (=1 Готово)

    // Физический интерфейс DRAM
    .dram_clk       (DRAM_CLK),       // Тактовая частота памяти
    .dram_ba        (DRAM_BA),        // 4 банка
    .dram_addr      (DRAM_ADDR),      // Максимальный адрес 2^13=8192
    .dram_dq        (DRAM_DQ),        // Ввод-вывод
    .dram_cas       (DRAM_CAS_N),     // CAS
    .dram_ras       (DRAM_RAS_N),     // RAS
    .dram_we        (DRAM_WE_N),      // WE
    .dram_ldqm      (DRAM_LDQM),      // Маска для младшего байта
    .dram_udqm      (DRAM_UDQM)       // Маска для старшего байта
);

endmodule
