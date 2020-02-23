#include "avr.h"

// Обработчик кадра
uint AVRDisplayTimer(uint interval, void *param) {

    SDL_Event     event;
    SDL_UserEvent userevent;

    /* Создать новый Event */
    userevent.type  = SDL_USEREVENT;
    userevent.code  = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return (interval);
}

// Конструктор
APP::APP(int w, int h, const char* caption) {

    width  = w;
    height = h;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_EnableUNICODE(1);

    sdl_screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption(caption, 0);

    // Инициализация
    pc = 0x0000;
    membank = 0;
    videomode = 0;

    ds_start = 0;
    ds_cursor = 0;
    ds_debugger = 1;
    ds_tab = 0;
    ds_dump_cursor = 0;
    ds_dump_start = 0;

    cpu_halt = 1;
    require_halt = 0;
    instr_counter = 0;
    count_per_frame = 200000;    // 10,0 mHz процессор
    framecycle = 0;
    ds_brk_cnt = 0;
    port_keyb_hit = 0;
    text_px = 0;
    text_py = 0;
    cursor_x = 0;
    cursor_y = 0;
    flash = 0;
    flash_id = 0;
    require_disp_update = 0;
    spi_status = 0;
    spi_command = 0;
    spi_phase = 0;
    spi_arg = 0;
    spi_lba = 0;
    spi_resp = 0xFF;
    mouse_cmd = 0;
    spi_st = 2; // Timeout SD

    sdram_data = (unsigned char*)malloc(64*1024*1024);
    for (int i = 0; i < 64*1024*1024; i++) sdram_data[i] = 0x55 ^ i;
}

APP::~APP() {
    free(sdram_data);
}

// Загрузка файла в память
void APP::loadfile(const char* fn) {

    FILE* fp = fopen(fn, "rb");
    if (fp) {

        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(program, 1, size, fp);
        fclose(fp);

    } else {

        printf("Указанный файл не был найден\n");
        exit(1);
    }
}

// Загрузка конфигурации
void APP::config() {

    int clock_mhz = 10,
        clock_video = 50;

    FILE* fp = fopen("config.ini", "r");

    if (fp) {

        fscanf(fp, "%d", & clock_mhz);
        fscanf(fp, "%d", & clock_video);

        // Инструкции за кадр
        count_per_frame = (clock_mhz * 1000000) / clock_video;

        fclose(fp);
    }

    SDL_AddTimer(1000 / clock_video, AVRDisplayTimer, NULL);
}

// ---------------------------------------------------------------------
// Главный обработчик приложения
// ---------------------------------------------------------------------

void APP::infinite() {

    int k, i, keyid, mk;

    while (1) {

        while (SDL_PollEvent(& event)) {

            switch (event.type) {

                // Если нажато на крестик, то приложение будет закрыто
                case SDL_QUIT:
                    return;

                case SDL_MOUSEMOTION:

                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    break;

                // Нажата мышь
                case SDL_MOUSEBUTTONDOWN: {

                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    mk      = event.motion.state;

                    if (mk == SDL_BUTTON_LEFT)       mouse_cmd |= 1;
                    else if (mk == SDL_BUTTON_RIGHT) mouse_cmd |= 2;

                    // Установка курсора мыши куда следует
                    if (mouse_x >= 8 && mouse_x < 54*8 && mouse_y >= 16 && mouse_y < 44*16 && ds_debugger) {

                        ds_cursor = ds_addresses[ mouse_y>>4 ];
                        swi_brk();
                        ds_update();
                    }

                    break;
                }

                // Мышь отпущена
                case SDL_MOUSEBUTTONUP:

                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    mk      = event.motion.state;

                    if (mk == SDL_BUTTON_LEFT)       mouse_cmd &= ~1;
                    else if (mk == SDL_BUTTON_RIGHT) mouse_cmd &= ~2;

                    break;

                // Нажата какая-то клавиша
                case SDL_KEYDOWN: {

                    keyid = get_key(event);

                    // Режим обычной работы процессора
                    if (!ds_debugger && !cpu_halt) {

                        if (keyid > 0) {

                            port_keyb_hit |= 1;
                            port_keyb_xt   = keyid;
                        }
                    }

                    // F7 Процессор должен быть остановлен для некоторых команд
                    if (cpu_halt) {

                        // Выполнить шаг
                        if (keyid == 0x41) { step(); ds_cursor = pc; }

                        // Если запущена отладка, обновить
                        if (ds_debugger) ds_update();
                    }

                    // F2
                    if (keyid == 0x3C && ds_debugger) {

                        swi_brk();
                        ds_update();
                    }

                    // F5 Посмотреть окно (рабочее) или отладчик
                    if (keyid == 0x3F) {

                        ds_debugger ^= 1;
                        update_screen();
                    }

                    // F9 Запуск программы (либо остановка)
                    if (keyid == 0x43) {

                        // Процессор остановлен, запустить его
                        if (cpu_halt) {

                            cpu_halt = 0;
                            ds_debugger = 0;
                            update_screen();
                        }
                        // Послать сигнал остановки
                        else {
                            require_halt = 1;
                        }
                    }

                    // +/- на NumPad
                    if (keyid == -82) { count_per_frame /= 1.5; printf("cycles: %d\n", count_per_frame); }
                    if (keyid == -86) { count_per_frame *= 1.5; printf("cycles: %d\n", count_per_frame); }

                    // TAB
                    if (keyid == 15) {

                        if (ds_debugger) {

                            // 0 - Отладчик
                            // 1 - Данные
                            ds_tab = (ds_tab + 1) % 2;
                            ds_update();
                        }
                    }

                    // PgDn
                    if (keyid == 0x56 && ds_debugger) {

                        if (ds_tab == 1) {
                            ds_dump_cursor += 0x1B0;
                            ds_dump_start  += 0x1B0;
                        }
                        else if (ds_tab == 0) {
                            ds_start += 88;
                            ds_cursor += 88;
                        }

                        ds_update();
                    }

                    // KeyDown
                    if (keyid == 0x62 && ds_debugger) {

                        if (ds_tab == 1) {

                            ds_dump_cursor += 0x010;
                            if (ds_dump_cursor >= ds_dump_start + 0x1B0)
                                ds_dump_start = ds_dump_cursor;
                        }
                        else if (ds_tab == 0) {

                            ds_cursor += 2;
                        }

                        ds_update();
                    }

                    // KeyUp
                    if (keyid == 0x60 && ds_debugger) {

                        if (ds_tab == 1) {

                            ds_dump_cursor -= 0x010;
                            if (ds_dump_cursor < 0)
                                ds_dump_cursor = 0;

                            if (ds_dump_cursor < ds_dump_start)
                                ds_dump_start = ds_dump_cursor;
                        }
                        else if (ds_tab == 0) {

                            ds_cursor -= 2;
                            if (ds_cursor < 0) ds_cursor = 0;
                        }

                        ds_update();
                    }

                    // PgUp
                    if (keyid == 0x55 && ds_debugger) {

                        if (ds_tab == 1) {
                            ds_dump_cursor -= 0x1B0;
                            ds_dump_start  -= 0x1B0;
                        }
                        else if (ds_tab == 0) {

                            ds_start -= 88;
                            ds_cursor -= 88;

                            if (ds_start < 0) ds_start = 0;
                            if (ds_cursor < 0) ds_cursor = 0;
                        }

                        ds_update();
                    }

                    // Горячие клавиши
                    if (ds_debugger && ds_tab == 1) {

                        if (keyid == 19) /* R */ ds_dump_cursor = ds_dump_start = 0x0000;
                        if (keyid == 47) /* V */ ds_dump_cursor = ds_dump_start = 0xc000;

                        ds_update();
                    }

                    // Нажатие горящей путевки в режиме ds_tab
                    if (ds_debugger && ds_tab == 0) {

                        if (keyid == 19) /* R */ { pc = ds_start = ds_cursor = 0; instr_counter = 0; ds_update(); }
                    }

                    break;
                }

                // Отпущена клавиша
                case SDL_KEYUP: {

                    keyid = get_key(event);

                    // Клавиша отпускается
                    if (!ds_debugger && !cpu_halt) {

                        if (keyid > 0) {

                            port_keyb_hit |= 1;
                            port_keyb_xt   = 0x80 | keyid;
                        }
                    }

                    break;
                }

                // Вызывается по таймеру
                case SDL_USEREVENT: {

                    timer = (timer + 20);
                    require_disp_update = 0;

                    flash_id++;
                    if (flash_id > 10) { flash_id = 0; flash ^= 1; require_disp_update = 1; }

                    // Если запрошена остановка
                    if (require_halt) {

                        cpu_halt = 1;
                        ds_debugger = 1;
                        ds_cursor = pc;
                        update_screen();
                    }

                    // Исполнить код, если процессор запущен
                    if (cpu_halt == 0) {

                        // Выполнить инструкции
                        while (framecycle < count_per_frame) {

                            // Выполнение инструкции
                            framecycle += step();

                            // Проверка останова
                            for (k = 0; k < ds_brk_cnt; k++) if (pc == ds_brk[k]) { cpu_halt = 1; break; }

                            // Отладочная инструкция BREAK
                            if (cpu_halt) break;
                        }

                        // Остановка процессора
                        if (cpu_halt) {

                            ds_debugger = 1;
                            ds_cursor = pc;
                            update_screen();
                        }

                        // Вращение остаточных
                        framecycle %= count_per_frame;
                    }

                    require_halt = 0;

                    // Запрос обновления экрана в режиме экрана (flash)
                    if (require_disp_update && !ds_debugger && videomode == 0) { display_update(); }

                    flip();
                    break;
                }
            }
        }

        SDL_Delay(1);
    }
}

// Положение курсора мыши X
int APP::get_mouse_x() {

    switch (videomode) {

        case 0: return mouse_x >> 4; break; // 80 x 25
        case 3: return mouse_x >> 2; break; // 320 x 200
    }

    return 0;
}

// Положение курсора мыши Y
int APP::get_mouse_y() {

    switch (videomode) {

        case 0: return mouse_y >> 5; break; // 80 x 25
        case 3: return mouse_y >> 2; break; // 320 x 200
    }

    return 0;
}

// Установить или удалить brkpoint
void APP::swi_brk() {

    int dsdel = 0; // Маркер удаления из
    for (int k = 0; k < ds_brk_cnt; k++) {

        // Удалить, if has
        if (ds_brk[k] == ds_cursor) {

            ds_brk_cnt--;
            for (int i = k; i < ds_brk_cnt; i++) {
                ds_brk[i] = ds_brk[i+1];
            }
            dsdel = 1;
            break;
        }
    }

    // Добавить точку останова, если нет ее
    if (dsdel == 0) ds_brk[ds_brk_cnt++] = ds_cursor;
}

// Получение XT-кода
int APP::get_key(SDL_Event event) {

    /* Получение ссылки на структуру с данными о нажатой клавише */
    SDL_KeyboardEvent * eventkey = & event.key;

    int xt = 0;
    int k = eventkey->keysym.scancode;

    //printf("%d ", k);
    switch (k) {

        /* A */ case 0x26: xt = 0x1E; break;
        /* B */ case 0x38: xt = 0x30; break;
        /* C */ case 0x36: xt = 0x2E; break;
        /* D */ case 0x28: xt = 0x20; break;
        /* E */ case 0x1a: xt = 0x12; break;
        /* F */ case 0x29: xt = 0x21; break;
        /* G */ case 0x2a: xt = 0x22; break;
        /* H */ case 0x2b: xt = 0x23; break;
        /* I */ case 0x1f: xt = 0x17; break;
        /* J */ case 0x2c: xt = 0x24; break;
        /* K */ case 0x2d: xt = 0x25; break;
        /* L */ case 0x2e: xt = 0x26; break;
        /* M */ case 0x3a: xt = 0x32; break;
        /* N */ case 0x39: xt = 0x31; break;
        /* O */ case 0x20: xt = 0x18; break;
        /* P */ case 0x21: xt = 0x19; break;
        /* Q */ case 0x18: xt = 0x10; break;
        /* R */ case 0x1b: xt = 0x13; break;
        /* S */ case 0x27: xt = 0x1F; break;
        /* T */ case 0x1c: xt = 0x14; break;
        /* U */ case 0x1e: xt = 0x16; break;
        /* V */ case 0x37: xt = 0x2F; break;
        /* W */ case 0x19: xt = 0x11; break;
        /* X */ case 0x35: xt = 0x2D; break;
        /* Y */ case 0x1d: xt = 0x15; break;
        /* Z */ case 0x34: xt = 0x2C; break;

        /* 0 */ case 0x13: xt = 0x0B; break;
        /* 1 */ case 0x0A: xt = 0x02; break;
        /* 2 */ case 0x0B: xt = 0x03; break;
        /* 3 */ case 0x0C: xt = 0x04; break;
        /* 4 */ case 0x0D: xt = 0x05; break;
        /* 5 */ case 0x0E: xt = 0x06; break;
        /* 6 */ case 0x0F: xt = 0x07; break;
        /* 7 */ case 0x10: xt = 0x08; break;
        /* 8 */ case 0x11: xt = 0x09; break;
        /* 9 */ case 0x12: xt = 0x0A; break;

        /* ~ */ case 0x31: xt = 0x29; break;
        /* - */ case 0x14: xt = 0x0C; break;
        /* = */ case 0x15: xt = 0x0D; break;
        /* \ */ case 0x33: xt = 0x2B; break;
        /* [ */ case 0x22: xt = 0x1A; break;
        /* ] */ case 0x23: xt = 0x1B; break;
        /* ; */ case 0x2f: xt = 0x27; break;
        /* ' */ case 0x30: xt = 0x28; break;
        /* , */ case 0x3b: xt = 0x33; break;
        /* . */ case 0x3c: xt = 0x34; break;
        /* / */ case 0x3d: xt = 0x35; break;

        /* bs */ case 0x16: xt = 0x0E; break; // Back Space
        /* sp */ case 0x41: xt = 0x39; break; // Space
        /* tb */ case 0x17: xt = 0x0F; break; // Tab
        /* ls */ case 0x32: xt = 0x2A; break; // Left Shift
        /* lc */ case 0x25: xt = 0x1D; break; // Left Ctrl
        /* la */ case 0x40: xt = 0x38; break; // Left Alt
        /* en */ case 0x24: xt = 0x1C; break; // Enter
        /* es */ case 0x09: xt = 0x01; break; // Escape

        /* F1  */ case 67: xt = 0x3B; break;
        /* F2  */ case 68: xt = 0x3C; break;
        /* F3  */ case 69: xt = 0x3D; break;
        /* F4  */ case 70: xt = 0x3E; break;
        /* F5  */ case 71: xt = 0x3F; break;
        /* F6  */ case 72: xt = 0x40; break;
        /* F7  */ case 73: xt = 0x41; break;
        /* F8  */ case 74: xt = 0x42; break;
        /* F9  */ case 75: xt = 0x43; break;
        /* F10 */ case 76: xt = 0x44; break;
        /* F11 */ case 95: xt = 0x57; break; // Не проверено
        /* F12 */ case 96: xt = 0x58; break;

        // ---------------------------------------------
        // Специальные (не так же как в реальном железе)
        // ---------------------------------------------

        /* UP  */  case 0x6F: xt = 0x60; break;
        /* RT  */  case 0x72: xt = 0x61; break;
        /* DN  */  case 0x74: xt = 0x62; break;
        /* LF  */  case 0x71: xt = 0x64; break;
        /* Home */ case 0x6E: xt = 0x6E; break;
        /* End  */ case 0x73: xt = 0x6F; break;
        /* PgUp */ case 0x70: xt = 0x55; break;
        /* PgDn */ case 0x75: xt = 0x56; break;
        /* Del  */ case 0x77: xt = 0x59; break;
        /* Ins  */ case 0x76: xt = 0x5A; break;
        /* NLock*/ case 0x4D: xt = 0x45; break;
        /* Esc  */ case 0x08: xt = 0x01; break;

        default: return -k;
    }

    /* Получить скан-код клавиш */
    return xt;
}
