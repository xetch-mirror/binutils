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

/* ---- freestanding replacements — no libc ---- */

static int my_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void my_strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static void my_strncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n && src[i]) { dst[i] = src[i]; i++; }
    while (i < n) { dst[i] = '\0'; i++; }
}

static int my_isdigit(char c) { return c >= '0' && c <= '9'; }
static int my_isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static int my_isalnum(char c) { return my_isdigit(c) || my_isalpha(c); }

static long my_atol(const char *s) {
    long result = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (my_isdigit(*s)) { result = result * 10 + (*s - '0'); s++; }
    return neg ? -result : result;
}

/* lexer.c */
void