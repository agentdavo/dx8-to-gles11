#include "dx8asm_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int asm_parse(const char *src, asm_program *prog, char **err) {
    (void)err;
    prog->code = NULL;
    prog->consts = NULL;
    prog->count = prog->capacity = 0;
    prog->const_count = prog->const_capacity = 0;
    const char *cur = src;
    while (*cur) {
        while (*cur == '\n' || *cur == '\r')
            ++cur;
        const char *ls = cur;
        while (*cur && *cur != '\n')
            ++cur;
        size_t len = cur - ls;
        if (*cur == '\n')
            ++cur;
        if (len == 0)
            continue;
        char *buf = util_strndup(ls, len);
        char *sc = strchr(buf, ';');
        if (sc)
            *sc = '\0';

        char *trim = buf;
        while (isspace((unsigned char)*trim))
            ++trim;

        if (!strncmp(trim, "def", 3)) {
            asm_constant c = {0};
            if (sscanf(trim, "def c%u, %f, %f, %f, %f", &c.idx, &c.value[0], &c.value[1],
                       &c.value[2], &c.value[3]) == 5) {
                sb_push(prog->consts, c);
            }
        } else {
            asm_instr inst = {0};
            if (sscanf(buf, "%7s %31[^,], %31[^,], %31s", inst.opcode, inst.dst,
                       inst.src0, inst.src1) >= 1) {
                sb_push(prog->code, inst);
            }
        }
        free(buf);
    }
    prog->count = sb_count(prog->code);
    prog->capacity = sb_capacity(prog->code);
    prog->const_count = sb_count(prog->consts);
    prog->const_capacity = sb_capacity(prog->consts);
    return 0;
}
void asm_program_free(asm_program *p) {
    sb_free(p->code);
    sb_free(p->consts);
    p->code = NULL;
    p->consts = NULL;
    p->count = p->capacity = 0;
    p->const_count = p->const_capacity = 0;
}
