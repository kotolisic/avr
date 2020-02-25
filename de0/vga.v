
// ---------------------------------------------------------------------
// Модуль видеоадаптера
// ---------------------------------------------------------------------

module vga(

    // 25 мегагерц
    input   wire        CLK25,
    input   wire [1:0]  vmode,

    // Выходные данные
    output  reg  [3:0]  VGA_R,      // 4 бит на красный (4,3,2,1,0)
    output  reg  [3:0]  VGA_G,      // 4 бит на зеленый (5,4,3,2,1,0)
    output  reg  [3:0]  VGA_B,      // 4 бит на синий (4,3,2,1,0)
    output  wire        VGA_HS,     // синхросигнал горизонтальной развертки
    output  wire        VGA_VS,     // синхросигнал вертикальной развертки

    output  reg  [11:0] font_addr,  // 2^12 = 4096 байт
    input   wire [7:0]  font_data,  // полученные данные от знакогенератора
    output  reg  [11:0] char_addr, // Указатель в видеопамять
    input   wire [7:0]  char_data, // Значение из видеопамяти

    input   wire [7:0]  cursor_x,
    input   wire [7:0]  cursor_y,

    // Видеографический адаптер
    output  reg  [14:0] gm_address,
    input   wire [ 7:0] gm_data
);

// ---------------------------------------------------------------------

// Тайминги для горизонтальной развертки (640)
parameter horiz_visible = 640;
parameter horiz_back    = 48;
parameter horiz_sync    = 96;
parameter horiz_front   = 16;
parameter horiz_whole   = 800;

// Тайминги для вертикальной развертки (400)
//                               // 400  480
parameter vert_visible  = 400;   // 400  480
parameter vert_back     = 35;    // 35   33
parameter vert_sync     = 2;     // 2    2
parameter vert_front    = 12;    // 12   10
parameter vert_whole    = 449;   // 449  525

// ---------------------------------------------------------------------

// 640 (видимая область) + 48 (задний порожек) + 96 (синхронизация) + 16 (передний порожек)
// 640 + 48 = [688, 688 + 96 = 784]
assign VGA_HS = x >= (horiz_visible + horiz_front) && x < (horiz_visible + horiz_front + horiz_sync);
assign VGA_VS = y >= (vert_visible  + vert_front)  && y < (vert_visible  + vert_front  + vert_sync);

// Положение луча
reg  [9:0] x;
reg  [9:0] y;
wire [9:0] x_real   = x > 791 ? x - 792 : x + 8;
wire [9:0] y_real   = x > 791 ? y + 1 : y;
wire [9:0] x_gr     = x > 797 ? x - 798 : x + 2;
wire [9:0] y_gr     = x > 797 ? y + 1 : y;

// Вычисление позиции курсора, его наличие.
wire       cursor   = ((cursor_x + 1 == x_real[9:3]) && (cursor_y == y_real[9:4])) && (y_real[3:0] >= 14);

// Признак завершения развертки
wire       x_stop   = x == (horiz_whole - 1);
wire       y_stop   = y == (vert_whole - 1);

// Объявим регистры со временными данными
reg  [7:0] char;
reg  [7:0] data;
reg  [7:0] attr;
reg        flash;
reg  [7:0] gm_color;

wire cubit = data[ 3'h7 ^ x_real[2:0] ];

// Конечный цвет
wire [11:0] color = cubit ^ (cursor & flash) ? fore_cl : back_cl;

// Графический режим, цвет
wire [ 3:0] gc4   = x_gr[1] ? gm_color[7:4] : gm_color[3:0];

// Выбор цвета из палитры (жестко заданной)
wire [11:0] gcl   = gc4 == 4'h0 ? 12'h000 :
                    gc4 == 4'h1 ? 12'h008 :
                    gc4 == 4'h2 ? 12'h080 :
                    gc4 == 4'h3 ? 12'h088 :
                    gc4 == 4'h4 ? 12'h800 :
                    gc4 == 4'h5 ? 12'h808 :
                    gc4 == 4'h6 ? 12'h880 :
                    gc4 == 4'h7 ? 12'hCCC :
                    gc4 == 4'h8 ? 12'h666 :
                    gc4 == 4'h9 ? 12'h00F :
                    gc4 == 4'hA ? 12'h0F0 :
                    gc4 == 4'hB ? 12'h0FF :
                    gc4 == 4'hC ? 12'hF00 :
                    gc4 == 4'hD ? 12'hF0F :
                    gc4 == 4'hE ? 12'hFF0 :
                                  12'hFFF;

// ---------------------------------------------------------------------
reg [23:0]  clock;
wire        clock_tick = (clock == 6250000);

// Таймер, 0.5 с
always @(posedge CLK25) begin

    flash <= clock_tick ? ~flash : flash;
    clock <= clock_tick ? 1'b0   : clock + 1;

end

// Пользовательские цвета | Теневые регистры
reg [11:0] fore_cl; reg [11:0] fore_cl_;
reg [11:0] back_cl; reg [11:0] back_cl_;
// ---------------------------------------------------------------------

always @(posedge CLK25) begin

    // Кадровая развертка
    x <= x_stop ?           1'b0 : x + 1'b1;
    y <= x_stop ? (y_stop ? 1'b0 : y + 1'b1) : y;

    // Генерация данных для последующего рисования
    case (x_real[2:0])

        // Считывание символа
        3'b000: begin char_addr      <= 2*(x_real[9:3] + 80*y_real[9:4]); end

        // Считывание атрибута, запись символа
        3'b001: begin char_addr      <= {char_addr[11:1], 1'b1};
                      char[7:0]      <= char_data; end

        // Запись атрибута
        3'b010: begin char_addr      <= {12'hFA0 + 2*char_data[3:0]};
                      attr[7:0]      <= char_data; end

        // Считывание номера цвета
        3'b011: begin char_addr      <= {char_addr[11:1], 1'b1};
                      fore_cl_[7:0]  <= char_data; end

        3'b100: begin char_addr      <= {12'hFA0 + 2*attr[7:4]};
                      fore_cl_[11:8] <= char_data[3:0]; end

        // Считывание знакоместа
        3'b101: begin char_addr      <= {char_addr[11:1], 1'b1};
                      back_cl_[7:0]  <= char_data; end

        3'b110: begin font_addr      <= {char, y_real[3:0]};
                      back_cl_[11:8] <= char_data[3:0]; end

        // Запись знакоместа
        3'b111: begin data    <= font_data;
                      fore_cl <= fore_cl_;
                      back_cl <= back_cl_; end

    endcase

    // Графический видеорежим
    case (x_gr[0])

        0: begin gm_address <= {160 * y_gr[8:1] + x_gr[9:2]}; end
        1: begin gm_color   <= gm_data; end

    endcase

    // Текстовый видеоадаптер выводит 640 x 400
    if (x < 640 && y < 400) begin

         // 80 x 25
         if (vmode == 0)      {VGA_R, VGA_G, VGA_B} <= color;
         // 320 x 200
         else if (vmode == 3) {VGA_R, VGA_G, VGA_B} <= gcl;

    end
    else {VGA_R, VGA_G, VGA_B} <= 12'h000;

end

endmodule
