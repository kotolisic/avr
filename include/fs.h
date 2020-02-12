// Информация о разделе
struct fs_drive {

    // Раздел
    unsigned long start;
    unsigned long size;
    unsigned char type;

    // Файловая система
    unsigned char cluster_size; // Размер кластера
    unsigned int  reserved;     // Резервированное место до FAT
    unsigned char fatnum;       // Количество FAT
    unsigned long fatsize;      // Размер FAT в секторах
    unsigned long root;         // Корневой каталог (кластер)
    unsigned long sector_fat;   // Абсолютный адрес сектора FAT
    unsigned long sector_data;  // Адрес сектора данных
};

struct FILE {

    unsigned char error;        // Есть ошибка
    unsigned long root;         // Первый кластер файла
    unsigned long seek;         // Курсор в файле
    unsigned long size;         // Размер файла
    unsigned char attr;         // Атрибуты файла
};

enum fs_type_enum {

    FS_NONE     = 0,
    FS_FAT32    = 0x0B
};

enum fs_errors_enum {

    FS_OK               = 0,
    FS_ERROR_UNDEFINED  = 1,
    FS_NOT_FOUND        = 2
};

// Временный блок памяти
unsigned char fs_tmp[512];

// Описание разделов
struct fs_drive drives[4];
unsigned char   drive_num, current_drive;
unsigned long   current_dir;

// Вспомогательные
char fs_filename[12];

// ---------------------------------------------------------------------
// Декларация
// ---------------------------------------------------------------------

void fs_init();
struct FILE fopen(char*);
