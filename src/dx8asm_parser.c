#include "dx8asm_parser.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim_ws(char *s) {
    while (isspace((unsigned char)*s))
        ++s;
    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    return s;
}

int asm_parse(const char *src, asm_program *prog, char **err) {
    prog->code = NULL;
    prog->consts = NULL;
    prog->count = prog->capacity = 0;
    prog->const_count = prog->const_capacity = 0;
    prog->type = ASM_SHADER_NONE;

    size_t line = 1;
    const char *cur = src;
    while (*cur) {
        while (*cur == '\n' || *cur == '\r') {
            ++cur;
            ++line;
        }
        const char *ls = cur;
        while (*cur && *cur != '\n')
            ++cur;
        size_t len = cur - ls;
        if (*cur == '\n') {
            ++cur;
            ++line;
        }
        if (len == 0)
            continue;

        char *buf = util_strndup(ls, len);
        char *sc = strchr(buf, ';');
        if (sc)
            *sc = '\0';

        char *trim = trim_ws(buf);
        if (*trim == '\0') {
            free(buf);
            continue; /* comment or blank line */
        }

        if (!strcmp(trim, "ps.1.1")) {
            prog->type = ASM_SHADER_PS11;
            free(buf);
            continue;
        }
        if (!strcmp(trim, "ps.1.3")) {
            prog->type = ASM_SHADER_PS13;
            free(buf);
            continue;
        }
        if (!strcmp(trim, "vs.1.1")) {
            prog->type = ASM_SHADER_VS11;
            free(buf);
            continue;
        }

        if (!strncmp(trim, "def", 3)) {
            asm_constant c = {0};
            if (sscanf(trim, "def c%u, %f, %f, %f, %f", &c.idx, &c.value[0],
                       &c.value[1], &c.value[2], &c.value[3]) == 5) {
                sb_push(prog->consts, c);
                free(buf);
                continue;
            }
            if (err)
                util_asprintf(err, "line %zu: invalid constant: %s", line - 1,
                              trim);
            free(buf);
            asm_program_free(prog);
            return -1;
        }

        asm_instr inst = {0};
        char operands[128] = "";
        if (sscanf(trim, "%15s%127[^\n]", inst.opcode, operands) < 1) {
            if (err)
                util_asprintf(err, "line %zu: invalid instruction: %s",
                              line - 1, trim);
            free(buf);
            asm_program_free(prog);
            return -1;
        }

        char *p = trim_ws(operands);
        if (*p) {
            char *save = NULL;
            char *tok = strtok_r(p, ",", &save);
            if (!tok) {
                if (err)
                    util_asprintf(err, "line %zu: invalid instruction: %s",
                                  line - 1, trim);
                free(buf);
                asm_program_free(prog);
                return -1;
            }
            tok = trim_ws(tok);
            if (strchr(tok, ' ') || strchr(tok, '\t')) {
                if (err)
                    util_asprintf(err, "line %zu: invalid instruction: %s",
                                  line - 1, trim);
                free(buf);
                asm_program_free(prog);
                return -1;
            }
            strncpy(inst.dst, tok, sizeof(inst.dst) - 1);

            tok = strtok_r(NULL, ",", &save);
            if (tok) {
                tok = trim_ws(tok);
                if (strchr(tok, ' ') || strchr(tok, '\t')) {
                    if (err)
                        util_asprintf(err,
                                      "line %zu: invalid instruction: %s",
                                      line - 1, trim);
                    free(buf);
                    asm_program_free(prog);
                    return -1;
                }
                strncpy(inst.src0, tok, sizeof(inst.src0) - 1);

                tok = strtok_r(NULL, ",", &save);
                if (tok) {
                    tok = trim_ws(tok);
                    if (strchr(tok, ' ') || strchr(tok, '\t')) {
                        if (err)
                            util_asprintf(err,
                                          "line %zu: invalid instruction: %s",
                                          line - 1, trim);
                        free(buf);
                        asm_program_free(prog);
                        return -1;
                    }
                    strncpy(inst.src1, tok, sizeof(inst.src1) - 1);

                    tok = strtok_r(NULL, ",", &save);
                    if (tok) {
                        tok = trim_ws(tok);
                        if (strchr(tok, ' ') || strchr(tok, '\t')) {
                            if (err)
                                util_asprintf(err,
                                              "line %zu: invalid instruction: %s",
                                              line - 1, trim);
                            free(buf);
                            asm_program_free(prog);
                            return -1;
                        }
                        strncpy(inst.src2, tok, sizeof(inst.src2) - 1);

                        if (strtok_r(NULL, ",", &save)) {
                            if (err)
                                util_asprintf(
                                    err,
                                    "line %zu: invalid instruction: %s",
                                    line - 1, trim);
                            free(buf);
                            asm_program_free(prog);
                            return -1;
                        }
                    }
                }
            }
        }

        sb_push(prog->code, inst);
        free(buf);
        continue;
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
