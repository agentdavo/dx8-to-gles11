#include "dx8asm_parser.h"
#include "dx8gles11.h"
#include "preprocess.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

static _Thread_local char g_err[256] = "";
static void set_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err, sizeof(g_err), fmt, ap);
    va_end(ap);
}
const char *dx8gles11_error(void) { return g_err; }

/* Command-list helpers */
static void cl_init(GLES_CommandList *l) {
    l->data = NULL;
    l->count = l->capacity = 0;
}
static void cl_push(GLES_CommandList *l, gles_cmd c) {
    sb_push(l->data, c);
    l->count = sb_count(l->data);
    l->capacity = sb_capacity(l->data);
}
void gles_cmdlist_free(GLES_CommandList *l) {
    sb_free(l->data);
    l->data = NULL;
    l->count = l->capacity = 0;
}
static int parse_stage(const char *reg, unsigned *out) {
    if (!reg || reg[0] != 't' || reg[1] < '0' || reg[1] > '3' || reg[2] != '\0')
        return -1;
    *out = (unsigned)(reg[1] - '0');
    return 0;
}
static void push4f(GLES_CommandList *o, gles_cmd_type t, float a, float b, float c, float d) {
    gles_cmd cmd = {.type = t};
    cmd.f[0] = a;
    cmd.f[1] = b;
    cmd.f[2] = c;
    cmd.f[3] = d;
    cl_push(o, cmd);
}

/* ----------------------------------------------------------------------------------

Opcode translators – extend as needed.

--------------------------------------------------------------------------------*/
static void xlate(const asm_instr *i, GLES_CommandList *o) {
    if (!strcmp(i->opcode, "mov") && !strcmp(i->dst, "oPos")) {
        gles_cmd c = {.type = GLES_CMD_VERTEX_ATTRIB};
        c.u[0] = 0;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "mov") && !strncmp(i->dst, "oD", 2)) {
        gles_cmd c = {.type = GLES_CMD_COLOR4F};
        c.u[0] = 0;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "dp4") && !strcmp(i->dst, "oPos")) {
        gles_cmd c = {.type = GLES_CMD_MATRIX_MODE};
        c.u[0] = GL_MODELVIEW;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "mul")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_MODULATE;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "sub")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_SUBTRACT;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "mad")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_ADD_SIGNED;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "lrp")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_INTERPOLATE;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "add")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_ADD;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "dp3")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_DOT3_RGB;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "mload")) {
        gles_cmd c = {.type = GLES_CMD_MATRIX_LOAD};
        c.f[0] = strtof(i->dst, NULL);
        c.f[1] = strtof(i->src0, NULL);
        c.f[2] = strtof(i->src1, NULL);
        c.f[3] = 1.0f;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "loadi")) {
        gles_cmd c = {.type = GLES_CMD_LOAD_IDENTITY};
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "tex")) {
        unsigned stage;
        if (parse_stage(i->dst, &stage)) {
            set_err("invalid tex stage: %s", i->dst);
            cl_push(o, (gles_cmd){.type = GLES_CMD_UNKNOWN});
            return;
        }
        gles_cmd c = {.type = GLES_CMD_TEX_SAMPLE};
        c.u[0] = stage;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "texld")) {
        unsigned stage;
        if (parse_stage(i->dst, &stage)) {
            set_err("invalid texld stage: %s", i->dst);
            cl_push(o, (gles_cmd){.type = GLES_CMD_UNKNOWN});
            return;
        }
        gles_cmd c = {.type = GLES_CMD_TEX_LOAD};
        c.u[0] = stage;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "texcrd")) {
        gles_cmd c = {.type = GLES_CMD_TEX_COORD_COPY};
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "texkill")) {
        gles_cmd c = {.type = GLES_CMD_TEX_KILL};
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "mov") && !strncmp(i->dst, "oT", 2)) {
        if (i->dst[2] < '0' || i->dst[2] > '7') {
            set_err("texture stage must be 0..7: %s", i->dst);
            cl_push(o, (gles_cmd){.type = GLES_CMD_UNKNOWN});
            return;
        }
        unsigned stage = (unsigned)(i->dst[2] - '0');
        gles_cmd c = {.type = GLES_CMD_MULTITEXCOORD4F};
        c.u[0] = GL_TEXTURE0 + stage;
        cl_push(o, c);
        return;
    }

    /* default */
    cl_push(o, (gles_cmd){.type = GLES_CMD_UNKNOWN});
}

/* shared compilation logic for string and file paths */
static int compile_from_source(const char *src, GLES_CommandList *out) {
    asm_program prog = {0};
    char *parse_err = NULL;
    if (asm_parse(src, &prog, &parse_err)) {
        set_err("parse error: %s", parse_err ? parse_err : "?");
        free(parse_err);
        return -1;
    }
    for (size_t idx = 0; idx < prog.count; ++idx)
        xlate(&prog.code[idx], out);

    asm_program_free(&prog);
    return 0;
}

int dx8gles11_compile_string(const char *src, const dx8gles11_options *opt,
                             GLES_CommandList *out) {
    (void)opt;
    if (!src) {
        set_err("source null");
        return -1;
    }
    if (!out) {
        set_err("out list null");
        return -1;
    }
    cl_init(out);
    return compile_from_source(src, out);
}

int dx8gles11_compile_file(const char *path, const dx8gles11_options *opt, GLES_CommandList *out) {
    if (!out) {
        set_err("out list null");
        return -1;
    }
    cl_init(out);

    char *pp_err = NULL;
    char *src = pp_run(path, opt ? opt->include_dir : NULL, &pp_err);
    if (!src) {
        set_err("preprocess fail: %s", pp_err ? pp_err : "?");
        free(pp_err);
        return -2;
    }

    asm_program prog = {0};
    char *parse_err = NULL;
    if (asm_parse(src, &prog, &parse_err)) {
        set_err("parse error: %s", parse_err ? parse_err : "?");
        free(parse_err);
        free(src);
        return -3;
    }
    for (size_t c = 0; c < prog.const_count; ++c) {
        gles_cmd cmd = {.type = GLES_CMD_LOAD_CONSTANT};
        cmd.u[0] = prog.consts[c].idx;
        cmd.f[0] = prog.consts[c].value[0];
        cmd.f[1] = prog.consts[c].value[1];
        cmd.f[2] = prog.consts[c].value[2];
        cmd.f[3] = prog.consts[c].value[3];
        cl_push(out, cmd);
    }
    for (size_t idx = 0; idx < prog.count; ++idx)
        xlate(&prog.code[idx], out);

    asm_program_free(&prog);
    free(src);
    return 0;
}
