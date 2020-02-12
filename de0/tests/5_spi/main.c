#include <avrio.c>
#include <textmode.c>
#include <fs.c>

int main() {

    cls(0x07);
    fs_init();

    struct FILE a = fopen("testx/hl3.txt");
    printint(a.size);

    // сменить текущий каталог
    // создать файл в каталоге
    // удалить файл (и каталог)
    // создать каталог
    // читать из файла 
    // писать в файл
    
    for(;;);
}
