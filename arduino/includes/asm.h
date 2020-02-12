#ifndef __ASM_INCLUDE
#define __ASM_INCLUDE

// Инструкции для работы
#define cli asm volatile("cli")
#define sei asm volatile("sei")
#define nop asm volatile("nop")

#endif
