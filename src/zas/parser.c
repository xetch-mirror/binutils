#include "as.h"

static void emit8(codebuf_t *c, uint8_t b)  { c->code[c->pos++] = b; }
static void emit32(codebuf_t *c, uint32_t v) {
    emit8(c, v & 0xFF); emit8(c, (v>>8)&0xFF);
    emit8(c, (v>>16)&0xFF); emit8(c, (v>>24)&0xFF);
}

static int find_symbol(symtab_t *s, const char *name) {
    int i;
    for (i = 0; i < s->sym_count; i++)
        if (my_strcmp(s->symbols[i].name, name) == 0) return i;
    return -1;
}

static int intern_symbol(symtab_t *s, const char *name) {
    int idx = find_symbol(s, name);
    if (idx >= 0) return idx;
    bob_symbol_t *sym = &s->symbols[s->sym_count];
    my_strncpy(sym->name, name, sizeof(sym->name) - 1);
    sym->name[sizeof(sym->name)-1] = '\0';
    sym->offset = 0;
    sym->defined = 0;
    return s->sym_count++;
}

static void add_reloc(reloctab_t *r, uint32_t offset, uint32_t sym_idx, uint8_t type) {
    r->relocs[r->reloc_count].offset = offset;
    r->relocs[r->reloc_count].symbol_index = sym_idx;
    r->relocs[r->reloc_count].type = type;
    r->reloc_count++;
}

static void pass1(token_stream_t *toks, symtab_t *syms) {
    int i = 0;
    uint32_t pc = 0;

    while (toks->tokens[i].type != TOK_EOF) {
        token_t *t = &toks->tokens[i];

        if (t->type == TOK_LABEL_DEF) {
            int idx = intern_symbol(syms, t->text);
            syms->symbols[idx].offset = pc;
            syms->symbols[idx].defined = 1;
            i++; continue;
        }
        if (t->type == TOK_NEWLINE) { i++; continue; }

        if (t->type == TOK_MNEMONIC) {
            if (!my_strcmp(t->text,"ret") || !my_strcmp(t->text,"nop")) pc += 1;
            else if (!my_strcmp(t->text,"push") || !my_strcmp(t->text,"pop")) pc += 1;
            else if (!my_strcmp(t->text,"int")) pc += 2;
            else if (!my_strcmp(t->text,"jmp") || !my_strcmp(t->text,"call")) pc += 5;
            else if (!my_strcmp(t->text,"je") || !my_strcmp(t->text,"jne")) pc += 6;
            else if (!my_strcmp(t->text,"mov")) {
                if (toks->tokens[i+2].type == TOK_IMMEDIATE) pc += 5;
                else pc += 2;
            }
            else if (!my_strcmp(t->text,"add") || !my_strcmp(t->text,"sub") || !my_strcmp(t->text,"cmp"))
                pc += 6;
        }
        i++;
    }
}

static void pass2(token_stream_t *toks, codebuf_t *code, symtab_t *syms, reloctab_t *relocs) {
    int i = 0;

    while (toks->tokens[i].type != TOK_EOF) {
        token_t *t = &toks->tokens[i];

        if (t->type == TOK_LABEL_DEF || t->type == TOK_NEWLINE) { i++; continue; }
        if (t->type != TOK_MNEMONIC) { i++; continue; }

        if (!my_strcmp(t->text, "ret"))  { emit8(code, 0xC3); i++; continue; }
        if (!my_strcmp(t->text, "nop"))  { emit8(code, 0x90); i++; continue; }

        if (!my_strcmp(t->text, "int")) {
            token_t *imm = &toks->tokens[i+1];
            emit8(code, 0xCD); emit8(code, (uint8_t)imm->value);
            i += 2; continue;
        }

        if (!my_strcmp(t->text, "push")) {
            token_t *r = &toks->tokens[i+1];
            emit8(code, 0x50 + (uint8_t)r->value);
            i += 2; continue;
        }
        if (!my_strcmp(t->text, "pop")) {
            token_t *r = &toks->tokens[i+1];
            emit8(code, 0x58 + (uint8_t)r->value);
            i += 2; continue;
        }

        if (!my_strcmp(t->text, "mov")) {
            token_t *dst = &toks->tokens[i+1];
            token_t *src = &toks->tokens[i+3];
            if (src->type == TOK_IMMEDIATE) {
                emit8(code, 0xB8 + (uint8_t)dst->value);
                emit32(code, (uint32_t)src->value);
            } else {
                emit8(code, 0x89);
                emit8(code, 0xC0 | ((uint8_t)src->value << 3) | (uint8_t)dst->value);
            }
            i += 4; continue;
        }

        if (!my_strcmp(t->text, "add") || !my_strcmp(t->text, "sub") || !my_strcmp(t->text, "cmp")) {
            token_t *dst = &toks->tokens[i+1];
            token_t *imm = &toks->tokens[i+3];
            uint8_t ext = !my_strcmp(t->text,"add") ? 0 : !my_strcmp(t->text,"sub") ? 5 : 7;
            emit8(code, 0x81);
            emit8(code, 0xC0 | (ext << 3) | (uint8_t)dst->value);
            emit32(code, (uint32_t)imm->value);
            i += 4; continue;
        }

        if (!my_strcmp(t->text, "jmp") || !my_strcmp(t->text, "call")) {
            token_t *lbl = &toks->tokens[i+1];
            int sidx = intern_symbol(syms, lbl->text);
            emit8(code, !my_strcmp(t->text,"jmp") ? 0xE9 : 0xE8);
            add_reloc(relocs, code->pos, sidx, 1);
            emit32(code, 0);
            i += 2; continue;
        }

        if (!my_strcmp(t->text, "je") || !my_strcmp(t->text, "jne")) {
            token_t *lbl = &toks->tokens[i+1];
            int sidx = intern_symbol(syms, lbl->text);
            emit8(code, 0x0F);
            emit8(code, !my_strcmp(t->text,"je") ? 0x84 : 0x85);
            add_reloc(relocs, code->pos, sidx, 1);
            emit32(code, 0);
            i += 2; continue;
        }

        i++;
    }
}

void assemble(token_stream_t *toks, codebuf_t *code, symtab_t *syms, reloctab_t *relocs) {
    pass1(toks, syms);
    pass2(toks, code, syms, relocs);

    int r;
    for (r = 0; r < relocs->reloc_count; r++) {
        bob_reloc_t *rel = &relocs->relocs[r];
        bob_symbol_t *sym = &syms->symbols[rel->symbol_index];
        if (sym->defined) {
            int32_t rel_val = (int32_t)sym->offset - (int32_t)(rel->offset + 4);
            code->code[rel->offset]   = rel_val & 0xFF;
            code->code[rel->offset+1] = (rel_val >> 8) & 0xFF;
            code->code[rel->offset+2] = (rel_val >> 16) & 0xFF;
            code->code[rel->offset+3] = (rel_val >> 24) & 0xFF;
        }
    }
}