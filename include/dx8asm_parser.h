#ifndef DX8ASM_PARSER_H
#define DX8ASM_PARSER_H
#include <stddef.h>

typedef struct asm_instr {
    char opcode[8], dst[32], src0[32], src1[32];
} asm_instr;

typedef struct asm_program {
    asm_instr *code;
    size_t count, capacity;
} asm_program;
int asm_parse(const char *src, asm_program *, char **err);
void asm_program_free(asm_program *);
#endif
