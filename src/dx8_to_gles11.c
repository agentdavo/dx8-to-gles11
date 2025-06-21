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

/* cache queried extension string */
static const char *g_extensions = NULL;

int dx8gles11_has_extension(const char *name) {
    if (!name)
        return 0;
    if (!g_extensions) {
        const GLubyte *ext = glGetString(GL_EXTENSIONS);
        if (!ext) {
            set_err("glGetString(GL_EXTENSIONS) failed");
            g_extensions = "";
            return 0;
        }
        g_extensions = (const char *)ext;
    }
    const char *s = g_extensions;
    size_t len = strlen(name);
    while (s && *s) {
        while (*s == ' ')
            ++s;
        if (!strncmp(s, name, len) && (s[len] == ' ' || s[len] == '\0'))
            return 1;
        s = strchr(s, ' ');
        if (!s)
            break;
        ++s;
    }
    set_err("missing GL extension: %s", name);
    return 0;
}

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

    if (!strcmp(i->opcode, "cnd")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_INTERPOLATE;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "nop")) {
        return; /* no-op */
    }

    if (!strcmp(i->opcode, "add")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_ADD;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "max")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_MAX_EXT;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "min")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_MIN_EXT;
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

    if (!strcmp(i->opcode, "texdp3")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_DOT3_RGB;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "texdp3tex")) {
        unsigned stage;
        if (parse_stage(i->dst, &stage)) {
            set_err("invalid texdp3tex stage: %s", i->dst);
            cl_push(o, (gles_cmd){.type = GLES_CMD_UNKNOWN});
            return;
        }
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_DOT3_RGB;
        cl_push(o, c);
        c = (gles_cmd){.type = GLES_CMD_TEX_SAMPLE};
        c.u[0] = stage;
        cl_push(o, c);
        return;
    }

    if (!strcmp(i->opcode, "texm3x3")) {
        gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
        c.u[0] = GL_COMBINE;
        c.u[1] = GL_DOT3_RGB;
        cl_push(o, c);
        cl_push(o, c);
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

/* validate instruction/constant limits for shader profiles */
static int validate_shader(const asm_program *p) {
    if (p->type == ASM_SHADER_PS11) {
        size_t tex = 0, arith = 0;
        for (size_t i = 0; i < p->count; ++i) {
            if (!strncmp(p->code[i].opcode, "tex", 3))
                ++tex;
            else
                ++arith;
        }
        if (tex > 4 || arith > 8) {
            set_err("ps.1.1 allows max 4 tex and 8 arithmetic instructions");
            return -1;
        }
    } else if (p->type == ASM_SHADER_PS13) {
        size_t tex = 0, arith = 0;
        for (size_t i = 0; i < p->count; ++i) {
            if (!strncmp(p->code[i].opcode, "tex", 3))
                ++tex;
            else
                ++arith;
        }
        if (tex > 4 || arith > 12) {
            set_err("ps.1.3 allows max 4 tex and 12 arithmetic instructions");
            return -1;
        }
    } else if (p->type == ASM_SHADER_VS11) {
        if (p->count > 128) {
            set_err("vs.1.1 allows max 128 instructions");
            return -1;
        }
        if (p->const_count > 12) {
            set_err("vs.1.1 allows max 12 constants");
            return -1;
        }
    }
    return 0;
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
    if (validate_shader(&prog)) {
        asm_program_free(&prog);
        return -2;
    }
    for (size_t idx = 0; idx < prog.count; ++idx)
        xlate(&prog.code[idx], out);

    asm_program_free(&prog);
    return 0;
}

int dx8gles11_compile_string(const char *src, const dx8gles11_options *opt,
                             GLES_CommandList *out) {
    if (!src) {
        set_err("source null");
        return -1;
    }
    if (!out) {
        set_err("out list null");
        return -1;
    }
    cl_init(out);

    char *pp_err = NULL;
    char *pp_src = pp_run_string(src, opt ? opt->include_dir : NULL, &pp_err);
    if (!pp_src) {
        set_err("preprocess fail: %s", pp_err ? pp_err : "?");
        free(pp_err);
        return -2;
    }

    asm_program prog = {0};
    char *parse_err = NULL;
    if (asm_parse(pp_src, &prog, &parse_err)) {
        set_err("parse error: %s", parse_err ? parse_err : "?");
        free(parse_err);
        free(pp_src);
        return -3;
    }
    if (validate_shader(&prog)) {
        asm_program_free(&prog);
        free(pp_src);
        return -4;
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
    free(pp_src);
    return 0;
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
    if (validate_shader(&prog)) {
        asm_program_free(&prog);
        free(src);
        return -4;
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
