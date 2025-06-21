#ifndef DX8ASM_PARSER_H
#define DX8ASM_PARSER_H
#include <stddef.h>

typedef struct asm_instr {
    /* opcode buffer must hold instructions like "texbeml" or longer */
    char opcode[16], dst[32], src0[32], src1[32], src2[32];
} asm_instr;

typedef enum asm_shader_type {
    ASM_SHADER_NONE,
    ASM_SHADER_PS11,
    ASM_SHADER_PS13,
    ASM_SHADER_VS11
} asm_shader_type;

typedef struct asm_constant {
    unsigned idx; /* cN register index */
    float value[4];
} asm_constant;

typedef struct asm_program {
    asm_shader_type type;
    asm_instr *code;
    size_t count, capacity;
    asm_constant *consts;
    size_t const_count, const_capacity;
} asm_program;
int asm_parse(const char *src, asm_program *, char **err);
void asm_program_free(asm_program *);
#endif
