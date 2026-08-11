#include "as.h"

static const char *regnames[] = {
    "eax","ecx","edx","ebx","esp","ebp","esi","edi", 0
};

int reg_id(const char *name) {
    int i;
    for (i = 0; regnames[i]; i++)
        if (my_strcmp(name, regnames[i]) == 0) return i;
    return -1;
}

static const char *mnemonics[] = {
    "mov","push","pop","add","sub","cmp",
    "jmp","je","jne","call","ret","int","nop", 0
};

static int is_mnemonic(const char *s) {
    int i;
    for (i = 0; mnemonics[i]; i++)
        if (my_strcmp(s, mnemonics[i]) == 0) return 1;
    return 0;
}

void lex(const char *src, token_stream_t *out) {
    const char *p = src;
    out->count = 0;

    while (*p) {
        if (*p == ' ' || *p == '\t') { p++; continue; }
        if (*p == ';') { while (*p && *p != '\n') p++; continue; }

        if (*p == '\n') {
            out->tokens[out->count++] = (token_t){TOK_NEWLINE, "\n", 0};
            p++; continue;
        }
        if (*p == ',') {
            out->tokens[out->count++] = (token_t){TOK_COMMA, ",", 0};
            p++; continue;
        }
        if (*p == '[') {
            out->tokens[out->count++] = (token_t){TOK_LBRACKET, "[", 0};
            p++; continue;
        }
        if (*p == ']') {
            out->tokens[out->count++] = (token_t){TOK_RBRACKET, "]", 0};
            p++; continue;
        }

        if (my_isdigit(*p) || (*p == '-' && my_isdigit(p[1]))) {
            char buf[MAX_LINE]; int i = 0;
            if (*p == '-') buf[i++] = *p++;
            while (my_isdigit(*p)) buf[i++] = *p++;
            buf[i] = '\0';
            token_t t = {TOK_IMMEDIATE, "", my_atol(buf)};
            my_strcpy(t.text, buf);
            out->tokens[out->count++] = t;
            continue;
        }

        if (my_isalpha(*p) || *p == '_' || *p == '.') {
            char buf[MAX_LINE]; int i = 0;
            while (my_isalnum(*p) || *p == '_' || *p == '.')
                buf[i++] = *p++;
            buf[i] = '\0';

            if (*p == ':') {
                p++;
                token_t t = {TOK_LABEL_DEF, "", 0};
                my_strcpy(t.text, buf);
                out->tokens[out->count++] = t;
                continue;
            }

            int rid = reg_id(buf);
            if (rid >= 0) {
                token_t t = {TOK_REGISTER, "", rid};
                my_strcpy(t.text, buf);
                out->tokens[out->count++] = t;
                continue;
            }

            token_type_t tt = is_mnemonic(buf) ? TOK_MNEMONIC : TOK_IDENT;
            token_t t = {tt, "", 0};
            my_strcpy(t.text, buf);
            out->tokens[out->count++] = t;
            continue;
        }

        p++; /* skip unknown char */
    }

    out->tokens[out->count++] = (token_t){TOK_EOF, "", 0};
}