#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>

#define BOB_MAGIC 0x21424F42u /* "BOB!" */

typedef struct {
    uint32_t magic;
    uint32_t code_size;
    uint32_t sym_count;
    uint32_t reloc_count;
} bob_header_t;

typedef struct {
    char name[32];
    uint32_t offset;   /* offset in code, valid if defined == 1 */
    uint8_t defined;   /* 1 = defined here, 0 = external (needs resolving by linker) */
} bob_symbol_t;

typedef struct {
    uint32_t offset;       /* where in code to patch */
    uint32_t symbol_index; /* index into symbol table */
    uint8_t type;          /* 0 = abs32, 1 = rel32 */
} bob_reloc_t;

#endif