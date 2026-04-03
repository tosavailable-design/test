#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>

// Обертка для системного вызова, потому что glibc её не предоставляет
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, 
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(SYS_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int main() {
    struct perf_event_attr pe_ins, pe_cyc;
    int fd_ins, fd_cyc;
    long long count_ins, count_cyc;

    printf("[*] Настраиваю прямое обращение к счетчикам процессора (PMU)...\n");

    // 1. Настройка счетчика ИНСТРУКЦИЙ
    memset(&pe_ins, 0, sizeof(struct perf_event_attr));
    pe_ins.type = PERF_TYPE_HARDWARE;
    pe_ins.size = sizeof(struct perf_event_attr);
    pe_ins.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe_ins.disabled = 1;         // Не начинать счет сразу
    pe_ins.exclude_kernel = 1;   // Не считать ядро
    pe_ins.exclude_hv = 1;       // Не считать гипервизор

    // 2. Настройка счетчика ТАКТОВ (CYCLES)
    memset(&pe_cyc, 0, sizeof(struct perf_event_attr));
    pe_cyc.type = PERF_TYPE_HARDWARE;
    pe_cyc.size = sizeof(struct perf_event_attr);
    pe_cyc.config = PERF_COUNT_HW_CPU_CYCLES;
    pe_cyc.disabled = 1;
    pe_cyc.exclude_kernel = 1;
    pe_cyc.exclude_hv = 1;

    // Пытаемся открыть счетчики. ВОТ ЗДЕСЬ ЯДРО НАС ПОШЛЕТ, ЕСЛИ ДОСТУПА НЕТ!
    fd_ins = perf_event_open(&pe_ins, 0, -1, -1, 0);
    if (fd_ins == -1) {
        perror("[-] ОШИБКА: Доступ к счетчику инструкций заблокирован гипервизором");
        return 1;
    }

    fd_cyc = perf_event_open(&pe_cyc, 0, -1, -1, 0);
    if (fd_cyc == -1) {
        perror("[-] ОШИБКА: Доступ к счетчику тактов заблокирован гипервизором");
        return 1;
    }

    printf("[+] Ядро дало доступ! Сбрасываю счетчики и запускаю адский цикл...\n");

    ioctl(fd_ins, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_cyc, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_ins, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_cyc, PERF_EVENT_IOC_ENABLE, 0);

    /* ========================================================= */
    /* ТВОЙ ARM64 АССЕМБЛЕР */
    uint64_t iterations = 500000000ULL; // 500 миллионов итераций
    uint64_t a = 0, b = 0, c = 0, d = 0;
    
    for (uint64_t i = 0; i < iterations; i++) {
        __asm__ volatile (
            "add %x[a], %x[a], #1\n\t"
            "add %x[b], %x[b], #1\n\t"
            "add %x[c], %x[c], #1\n\t"
            "add %x[d], %x[d], #1\n\t"
            : [a] "+r" (a), [b] "+r" (b), [c] "+r" (c), [d] "+r" (d)
        );
    }
    /* ========================================================= */

    // Останавливаем счетчики
    ioctl(fd_ins, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd_cyc, PERF_EVENT_IOC_DISABLE, 0);

    // Читаем результаты
    read(fd_ins, &count_ins, sizeof(long long));
    read(fd_cyc, &count_cyc, sizeof(long long));

    printf("\n==== РЕЗУЛЬТАТЫ ====\n");
    printf("Магическое число (чтобы компилятор не вырезал код): %lu\n", a + b + c + d);
    printf("Аппаратных инструкций выполнено: %lld\n", count_ins);
    printf("Тактов процессора прошло:        %lld\n", count_cyc);
    
    if (count_cyc > 0) {
        double ipc = (double)count_ins / count_cyc;
        printf(">>> ТВОЙ IPC: %.3f инструкций за такт <<<\n", ipc);
    } else {
        printf("[-] Такты равны нулю. Гипервизор шлет фейковые данные.\n");
    }

    close(fd_ins);
    close(fd_cyc);
    return 0;
    }
