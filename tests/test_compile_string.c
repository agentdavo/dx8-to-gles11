#include "dx8gles11.h"
#include "utils.h"
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static void append(char **out, const char *text) {
    for (const char *p = text; *p; ++p)
        sb_push(*out, *p);
}

static char *dump_cmds(const GLES_CommandList *cl) {
    char *out = NULL;
    for (size_t i = 0; i < cl->count; ++i) {
        const gles_cmd *c = &cl->data[i];
        char *line = NULL;
        switch (c->type) {
        case GLES_CMD_COLOR4F:
            util_asprintf(&line, "COLOR4F\n");
            break;
        case GLES_CMD_MULTITEXCOORD4F:
            util_asprintf(&line, "MULTITEXCOORD4F stage=%u\n", c->u[0] - GL_TEXTURE0);
            break;
        case GLES_CMD_TEX_ENV_COMBINE:
            util_asprintf(&line, "TEX_ENV_COMBINE func=%u\n", c->u[1]);
            break;
        case GLES_CMD_TEX_SAMPLE:
            util_asprintf(&line, "TEX_SAMPLE stage=%u\n", c->u[0]);
            break;
        case GLES_CMD_VERTEX_ATTRIB:
            util_asprintf(&line, "VERTEX_ATTRIB idx=%u\n", c->u[0]);
            break;
        default:
            util_asprintf(&line, "UNKNOWN\n");
            break;
        }
        append(&out, line);
        free(line);
    }
    sb_push(out, '\0');
    return out;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s shader.asm expected.txt include_dir\n", argv[0]);
        return 1;
    }
    const char *asm_path = argv[1];
    const char *expect_path = argv[2];
    const char *inc_dir = argv[3];

    char *src = read_file(asm_path);
    if (!src) {
        fprintf(stderr, "could not read %s\n", asm_path);
        return 1;
    }

    dx8gles11_options opt = {.include_dir = inc_dir};
    GLES_CommandList cl;
    if (dx8gles11_compile_string(src, &opt, &cl) != 0) {
        fprintf(stderr, "%s\n", dx8gles11_error());
        free(src);
        return 1;
    }
    char *out = dump_cmds(&cl);
    char *expect = read_file(expect_path);
    int ok = expect && out && strcmp(out, expect) == 0;
    if (!ok) {
        fprintf(stderr, "expected:\n%s\n-----\nactual:\n%s\n", expect ? expect : "(null)", out ? out : "(null)");
    }
    free(expect);
    free(src);
    sb_free(out);
    gles_cmdlist_free(&cl);
    return ok ? 0 : 1;
}
