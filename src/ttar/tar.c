#include "syscalls.h"
#include "tar.h"

int main(int argc, char **argv) {
    char *archive_path = 0;
    int action = 0; /* 1 - pack ('c'), 2 - unpack ('x') */

    if (argc < 3) {
        print_str("Usage:\n  ttar c <archive.tar> <files...>\n  ttar x <archive.tar>\n");
        return 1;
    }

    if (argv[1][0] == 'c' && argv[1][1] == '\0') {
        action = 1;
    } else if (argv[1][0] == 'x' && argv[1][1] == '\0') {
        action = 2;
    } else return 1;

    archive_path = argv[2];

    if (action == 1) {
        return create_archive(archive_path, argc - 3, argv + 3);
        }

    return extract_archive(archive_path);

    /* Avoided writing return 0; because it allows for (Tail Call Optimization) */

}

#if defined(__x86_64__)
void _start(void) {
    __asm__ __volatile__(
        "movq (%rsp), %rdi\n"    /* argc */
        "leaq 8(%rsp), %rsi\n"   /* argv */
        "call main\n"
        "movq %rax, %rdi\n"      /* status */
        "movq $60, %rax\n"       /* sys_exit */
        "syscall\n"
    );
}

#elif defined(__i386__)
void _start(void) {
    __asm__ __volatile__(
        "popl %eax\n"            /* argc -> eax */
        "movl %esp, %ebx\n"      /* argv -> ebx */
        "pushl %ebx\n"
        "pushl %eax\n"
        "call main\n"
        "movl %eax, %ebx\n"      /* status -> ebx */
        "movl $1, %eax\n"        /* sys_exit */
        "int $0x80\n"
    );
}

#elif defined(__aarch64__)
void _start(void) {
    __asm__ __volatile__(
        "ldr x0, [sp]\n"         /* argc -> x0 */
        "add x1, sp, #8\n"       /* argv -> x1 */
        "bl main\n"
        "mov x0, x0\n"           /* status -> x0 */
        "mov x8, #93\n"          /* sys_exit */
        "svc #0\n"
    );
}

#elif defined(__arm__)
void _start(void) {
    __asm__ __volatile__(
        "ldr r0, [sp]\n"         /* argc -> r0 */
        "add r1, sp, #4\n"       /* argv -> r1 */
        "bl main\n"
        "mov r0, r0\n"           /* status -> r0 */
        "mov r7, #1\n"           /* sys_exit */
        "swi 0\n"
    );
}

#elif defined(__mips__)
void _start(void) {
    __asm__ __volatile__(
        "lw $a0, 0($sp)\n"       /* argc -> a0 */
        "addiu $a1, $sp, 4\n"    /* argv -> a1 */
        "jal main\n"
        "move $a0, $v0\n"        /* status -> a0 */
        "li $v0, 4001\n"         /* sys_exit */
        "syscall\n"
    );
}

#elif defined(__riscv)
void _start(void) {
    __asm__ __volatile__(
        "ld a0, 0(sp)\n"         /* argc -> a0 */
        "addi a1, sp, 8\n"       /* argv -> a1 */
        "jal main\n"
        "mv a0, a0\n"            /* status -> a0 */
        "li a7, 93\n"            /* sys_exit */
        "ecall\n"
    );
}
#endif