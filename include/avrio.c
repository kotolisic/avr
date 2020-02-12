// Ссылка на пустой адрес
#define NULL    ((void*)0)
#define brk     asm volatile("sleep"); // break

#define byte    unsigned char
#define uint    unsigned int
#define word    unsigned int
#define ulong   unsigned long
#define dword   unsigned long

// Чтение из порта
inline unsigned char inp(int port) { return ((volatile unsigned char*)0x20)[port]; }

// Запись в порт
inline void outp(int port, unsigned char val) { ((volatile unsigned char*)0x20)[port] = val; }

// Положение мыши
inline int get_mouse_x() { return inp(0xA) | (inp(0xE) << 8); }
inline int get_mouse_y() { return inp(0xB); }
inline int get_mouse_st() { return inp(0xC); }


