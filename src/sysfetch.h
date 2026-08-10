#ifndef SYSFETCH_H
#define SYSFETCH_H

#include <stdint.h>
#include <stddef.h>

/* Hook function pointer type for character output.
 * Provide your kernel/freestanding console putchar (e.g., vga_putchar or serial_putc). */
typedef void (*sysfetch_putchar_t)(char c);

/* Structure to pass hardware/system stats into sysfetch */
typedef struct {
    const char *os_name;      /* e.g., "MyOS x86_64" */
    const char *kernel_ver;   /* e.g., "0.1.0-freestanding" */
    uint64_t uptime_secs;     /* Total system uptime in seconds */
    uint64_t mem_used_bytes;  /* RAM used in bytes */
    uint64_t mem_total_bytes; /* Total RAM in bytes */
} sysfetch_info_t;

/* Main entry point for the utility */
void sysfetch_print(sysfetch_putchar_t putc, const sysfetch_info_t *info);

#endif /* SYSFETCH_H */
