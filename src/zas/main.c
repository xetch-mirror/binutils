#include "syscalls.h"
#include "as.h"

#define MAX_SRC_SIZE (256 * 1024)

static char src_buf[MAX_SRC_SIZE];

static void print_str(const char *msg) {
    const char *p = msg;
    while (*p) p++;
    sys_write(2, (void *)msg, (unsigned long)(p - msg));
}

static long read_whole_file(const char *path, char *buf, long max_size) {
    int fd = sys_open((char *)path, O_RDONLY, 0);
    if (fd < 0) return -1;

    long total = 0;
    long n;
    while (total < max_size &&
           (n = sys_read(fd, buf + total, (unsigned long)(max_size - total))) > 0) {
        total += n;
    }
    sys_close(fd);
    buf[total] = '\0';
    return total;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_str("Usage: as <input.asm> <output.o>\n");
        return 1;
    }

    long src_len = read_whole_file(argv[1], src_buf, MAX_SRC_SIZE - 1);
    if (src_len < 0) {
        print_str("as: could not open input file\n");
        return 1;
    }

    static token_stream_t toks;
    static codebuf_t code;
    static symtab_t syms;
    static reloctab_t relocs;
    code.pos = 0; syms.sym_count = 0; relocs.reloc_count = 0;

    lex(src_buf, &toks);
    assemble(&toks, &code, &syms, &relocs);

    int out_fd = sys_open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        print_str("as: could not open output file\n");
        return 1;
    }

    bob_header_t hdr;
    hdr.magic = BOB_MAGIC;
    hdr.code_size = code.pos;
    hdr.sym_count = (uint32_t)syms.sym_count;
    hdr.reloc_count = (uint32_t)relocs.reloc_count;

    sys_write(out_fd, &hdr, sizeof(hdr));
    sys_write(out_fd, code.code, code.pos);
    sys_write(out_fd, syms.symbols, (unsigned long)(sizeof(bob_symbol_t) * syms.sym_count));
    sys_write(out_fd, relocs.relocs, (unsigned long)(sizeof(bob_reloc_t) * relocs.reloc_count));
    sys_close(out_fd);

    return 0;
}