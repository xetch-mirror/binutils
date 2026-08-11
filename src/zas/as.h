#ifndef AS_H
#define AS_H

#include <stdint.h>
#include "object.h"

#define MAX_TOKENS   65536
#define MAX_SYMBOLS  1024
#define MAX_RELOCS   1024
#define MAX_CODE     65536
#define MAX_LINE     256

typedef enum {
    TOK_MNEMONIC, TOK_REGISTER, TOK_IMMEDIATE,
    TOK_LABEL_DEF, TOK_IDENT, TOK_COMMA, TOK_LBRACKET,
    TOK_RBRACKET, TOK_NEWLINE, TOK_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    char text[MAX_LINE];
    long value; /* for TOK_IMMEDIATE / TOK_REGISTER (reg id) */
} token_t;

typedef struct {
    token_t tokens[MAX_TOKENS];
    int count;
} token_stream_t;

typedef struct {
    bob_symbol_t symbols[MAX_SYMBOLS];
    int sym_count;
} symtab_t;

typedef struct {
    bob_reloc_t relocs[MAX_RELOCS];
    int reloc_count;
} reloctab_t;

typedef struct {
    uint8_t code[MAX_CODE];
    uint32_t pos;
} codebuf_t;

/* lexer.c */
void lex(const char *src, token_stream_t *out);

/* parser.c */
void assemble(token_stream_t *toks, codebuf_t *code,
              symtab_t *syms, reloctab_t *relocs);

int reg_id(const char *name); /* returns -1 if not a register */

#endif