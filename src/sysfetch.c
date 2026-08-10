#include "sysfetch.h"

/* Color Macros matching the ZYTHOS image */
#define C_RING "\033[1;32m"       /* Bright Green for outer ring segments & center circle */
#define C_HEX  "\033[0;32m"       /* Darker Green for hexagon body */
#define C_TXT  "\033[1;30;47m"    /* Bold Black text on light background (or \033[1;30m) */
#define C_RST  "\033[0m"

static void sysfetch_puts(sysfetch_putchar_t putc, const char *s) {
    while (*s) {
        putc(*s++);
    }
}

static void sysfetch_put_uint(sysfetch_putchar_t putc, uint64_t n) {
    if (n == 0) { putc('0'); return; }
    char buf[21];
    int i = 0;
    while (n > 0) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) { putc(buf[--i]); }
}

static void get_cpu_brand(char out[49]) {
    uint32_t *b = (uint32_t *)out;
    for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
        __asm__ volatile (
            "cpuid"
            : "=a"(b[0]), "=b"(b[1]), "=c"(b[2]), "=d"(b[3])
            : "a"(leaf)
        );
        b += 4;
    }
    out[48] = '\0';
}

/* Colorized ZYTHOS Logo Line-by-Line */
static const char *colored_logo[] = {
    "       " C_RING ".:::--::." C_RST "                     ",
    "     " C_RING ".-=============-." C_RST "                 ",
    "   " C_RING "-====:" C_HEX " :==:." C_RING "   :====-" C_RST "               ",
    " " C_RING ":===:" C_HEX " .:+++++++=-:." C_RING " ::  :" C_RST "             ",
    "" C_RING ".:=:" C_HEX " .=++++++++++++++=-:" C_RING "===." C_RST "           ",
    "  " C_RING ".:" C_HEX "   :=++++++" C_RING "=====" C_HEX "+++++++=" C_RING ":===." C_RST "          ",
    "  " C_RING ":==:" C_HEX "++++++" C_RING "===========" C_HEX "+++++:" C_RING " ==-" C_RST "          ",
    " " C_RING ".==:." C_HEX "+++++" C_RING "=============" C_HEX "++++-" C_RING " :==." C_RST "         ",
    " " C_RING ":== " C_HEX ".++++" C_RING "===============" C_HEX "+++-" C_RING "  ==:" C_RST "         ",
    /* ZYTHOS Banner Line - Black characters */
    C_TXT "===*@@*+%%*#@%+*#%@**=@#==+@*%@%*+@%*@*-:==" C_RST "    ",
    C_TXT "*@*:::==..##+++==*%===@#==+@#%@**-%%+=:::*@" C_RST "   ",
    " " C_RING ".===" C_HEX " :++++" C_RING "=============" C_HEX "+++++:" C_RST "  .          ",
    "  " C_RING ":==::" C_HEX "=+++++" C_RING "=========" C_HEX "++++++" C_RING "::-:." C_RST "          ",
    "   " C_RING "." C_HEX "   =++++++++++++++++++." C_RING " :==-" C_RST "           ",
    "     " C_RING ":==." C_HEX " :=+++++++++++=." C_RING " .===:" C_RST "            ",
    "      " C_RING "-===:." C_HEX "  :==+++=-" C_RING " .:===-" C_RST "             ",
    "       " C_RING ".:====--::::::-====:." C_RST "               ",
    "           " C_RING ".-=====. :=-" C_RST "                   "
};

#define LOGO_ROWS 18

void sysfetch_print(sysfetch_putchar_t putc, const sysfetch_info_t *info) {
    char cpu_brand[49];
    get_cpu_brand(cpu_brand);

    for (int row = 0; row < LOGO_ROWS; row++) {
        /* Print line of Logo */
        sysfetch_puts(putc, colored_logo[row]);
        sysfetch_puts(putc, "   ");

        /* Print corresponding System Info */
        switch (row) {
            case 3:
                sysfetch_puts(putc, C_RING "OS:" C_RST " ");
                sysfetch_puts(putc, info->os_name ? info->os_name : "ZYTHOS OS");
                break;
            case 4:
                sysfetch_puts(putc, C_RING "Kernel:" C_RST " ");
                sysfetch_puts(putc, info->kernel_ver ? info->kernel_ver : "1.0");
                break;
            case 5:
                sysfetch_puts(putc, C_RING "CPU:" C_RST " ");
                sysfetch_puts(putc, cpu_brand);
                break;
            case 6: {
                uint64_t hrs  = info->uptime_secs / 3600;
                uint64_t mins = (info->uptime_secs % 3600) / 60;
                uint64_t secs = info->uptime_secs % 60;
                sysfetch_puts(putc, C_RING "Uptime:" C_RST " ");
                if (hrs > 0) { sysfetch_put_uint(putc, hrs); sysfetch_puts(putc, "h "); }
                sysfetch_put_uint(putc, mins); sysfetch_puts(putc, "m ");
                sysfetch_put_uint(putc, secs); sysfetch_puts(putc, "s");
                break;
            }
            case 7: {
                uint64_t used_mb  = info->mem_used_bytes  / (1024 * 1024);
                uint64_t total_mb = info->mem_total_bytes / (1024 * 1024);
                sysfetch_puts(putc, C_RING "Memory:" C_RST " ");
                sysfetch_put_uint(putc, used_mb);
                sysfetch_puts(putc, " MiB / ");
                sysfetch_put_uint(putc, total_mb);
                sysfetch_puts(putc, " MiB");
                break;
            }
            case 8:
                /* Color Palette Bar */
                sysfetch_puts(putc, "\033[40m  \033[42m  \033[46m  \033[47m  \033[102m  \033[0m");
                break;
            default:
                break;
        }

        sysfetch_puts(putc, "\n");
    }
}
