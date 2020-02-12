#include <sd.c>
#include "fs.h"

// Дамп сектора на экране
void fs_dump() {

    for (int i = 0; i < 512; i++) {
        printhex(fs_tmp[i], 1);
        printch(' ');
    }
}

// Сканирование разделов на жестком диске и определение файловых систем
void fs_init() {

    unsigned char i;

    drive_num = 0;

    // Прочитать 0-й сектор, разобрать MBR
    sd_read(0, fs_tmp);

    for (i = 0; i < 4; i++) {

        int start = 0x1BE + 16*i;

        // Обнаружена поддерживаемая файловая система
        if (fs_tmp[start + 4] == 0x0B) {

            drives[drive_num].type  = FS_FAT32;
            drives[drive_num].start = *(unsigned long*) (fs_tmp + start +  8);
            drives[drive_num].size  = *(unsigned long*) (fs_tmp + start + 12);
            drive_num++;
        }
    }

    // Просканировать каждую файловую систему
    for (i = 0; i < drive_num; i++) {

        sd_read(drives[i].start, fs_tmp);

        switch (drives[i].type) {

            case FS_FAT32:

                drives[i].cluster_size  = fs_tmp[0x0D];
                drives[i].fatnum        = fs_tmp[0x10];
                drives[i].reserved      = *(unsigned int*)  (fs_tmp + 0x0E);
                drives[i].fatsize       = *(unsigned long*) (fs_tmp + 0x24);
                drives[i].root          = *(unsigned long*) (fs_tmp + 0x2C);

                // Вычисление абсолюиной позиции FAT / DATA
                drives[i].sector_fat  = drives[i].start + drives[i].reserved;
                drives[i].sector_data = drives[i].sector_fat + (drives[i].fatnum * drives[i].fatsize);
                break;
        }
    }

    // По умолчанию будет всегда 1-й
    current_drive = 0;
    current_dir   = drives[0].root;
}

// Читать кластер посекторно в [fs_tmp]
// drive_id = 0..3
// cluster_id = 0..N
// at = 0..cluster_size-1
void fs_read_cluster(ulong cluster_id, byte at) {

    sd_read( drives[current_drive].sector_data +
            (drives[current_drive].cluster_size * (cluster_id - 2) +
             at), fs_tmp);
}

// Считать имя файла и нормализовать его. Если @return 0, файл последний
int fs_fetch_name(char* filename, int n) {

    int i, m;

    // Получение имени файла
    for (i = 0; i < 11; i++) fs_filename[i] = ' '; m = 0;

    // Исследование и нормализация
    for (i = 0; i < 12; i++) {

        char c = filename[n++];

        // Проверка достижения конца строки
        if (c == 0) { n = 0; break; }

        // Закончился ввод файла
        else if (c == '/' || c == '\\') {
            break;
        }
        // Передвинуть точку?
        else if (c == '.' && m <= 8) {
            m = 8;
        }
        // Пропечатать символ в верхнем регистре
        else {
            fs_filename[m++] = (c >= 'a' && c <= 'z') ? c + ('A' - 'a') : c;
        }
    }

    fs_filename[11] = 0;
    return n;
}

// По текущему кластеру получить следующий
ulong fs_get_next_cluster(ulong cluster_id) {

    // Извлекаем сектор
    sd_read(drives[current_drive].sector_fat + (cluster_id >> 7), fs_tmp);

    // Извлекаем номер следующего кластера (4 байта)
    ulong next_cluster = *(ulong*)(fs_tmp + (4 * (cluster_id & 0x7F)));

    return next_cluster;
}

// Просмотр файла `fs_filename` в запрошенном каталоге
// Если файл не был найден, то 0, иначе от 1 до 32 (номера слотов)
byte fs_search_in_catalog(unsigned long root) {

    unsigned char i, j, k;

    do { // Листание кластеров каталога

        // Чтение текущего каталога
        for (i = 0; i < drives[current_drive].cluster_size; i++) {

            // Прочесть сектор из кластера
            fs_read_cluster(root, i);

            // Просмотреть все файлы в секторе
            for (j = 0; j < 16; j++) {

                // Сравнить файл с запрошенным
                for (k = 0; k < 12; k++) {

                    // Файл был успешно найден
                    if (k == 11) return (1 + j);

                    // Файл не был найден
                    if (fs_tmp[j*32 + k] != fs_filename[k])
                        break;
                }
            }
        }

        // Поиск следующего кластера каталога
        root = fs_get_next_cluster(root);

    } while (root < 0x0FFFFFF0);

    // Отсмотрен каталог и файл не был найден
    return 0;
}

// Получение номера кластера
ulong fs_get_cluster_id(uint id) {

    ulong hi = *(word*) (fs_tmp + (id*32) + 0x14);
    ulong lo = *(word*) (fs_tmp + (id*32) + 0x1A);

    return (hi << 16) | lo;
}

// Поиск файла. Если найден, возвращается file_id = 1..32, не найден =0
byte fs_search_file(char* filename) {

    int  n = 0;
    byte attr;

    // Начать с текущего каталога
    ulong root    = current_dir;
    byte  file_id = 0;

    do {

        // Считать очередное имя файла
        n = fs_fetch_name(filename, n);

        // Просмотр файла `fs_filename` в запрошенном каталоге
        file_id = fs_search_in_catalog(root);

        // Ничего не найдено по итогу
        if (file_id == 0) return 0; else file_id--;

        // Атрибуты файла или директории
        attr = fs_tmp[ file_id*32 + 0x0B ];

        // Директория: сменить каталог
        if (attr & 0x10) root = fs_get_cluster_id(file_id);

    } while (n);

    // Файл был найден в итоге
    return (1 + file_id);
}

// Найти и открыть файл
struct FILE fopen(char* filename) {

    struct FILE file;

    file.error = FS_ERROR_UNDEFINED;
    file.root  = 0;
    file.size  = 0;

    // Поиск файла по имени файла, если не найден, то 0
    byte file_id = fs_search_file(filename);

    // Загрузка метаинформации о файле
    if (file_id) {

        file_id--;
        file.error = FS_OK;
        file.root  = fs_get_cluster_id(file_id);
        file.attr  = fs_tmp[32*file_id + 0x0B];
        file.seek  = 0;
        file.size  = *(ulong*) (fs_tmp + 32*file_id + 0x1C);

    } else {
        file.error = FS_NOT_FOUND;
    }

    return file;
}
