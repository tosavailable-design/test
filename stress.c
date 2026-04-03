#include <stdio.h>
#include <stdint.h>

int main() {
    // Делаем миллиард итераций
    uint64_t iterations = 1000000000ULL;
    uint64_t a = 0, b = 0, c = 0, d = 0;

    printf("Начинаю насиловать ARM-процессор независимыми инструкциями...\n");

    for (uint64_t i = 0; i < iterations; i++) {
        /* 
           ARM64 Ассемблер. 
           Формат: add приемник, операнд1, #число 
           %x означает использование 64-битных x-регистров.
        */
        __asm__ volatile (
            "add %x[a], %x[a], #1\n\t"
            "add %x[b], %x[b], #1\n\t"
            "add %x[c], %x[c], #1\n\t"
            "add %x[d], %x[d], #1\n\t"
            : [a] "+r" (a), [b] "+r" (b), [c] "+r" (c), [d] "+r" (d)
        );
    }

    printf("Готово. Магическое число: %lu\n", a + b + c + d);
    
    return 0;
}
