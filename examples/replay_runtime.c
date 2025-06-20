#include "dx8gles11.h"
#include <stdio.h>

#include <GLES/gl.h>

static void execute_cmds(const GLES_CommandList *cl) {
    for (size_t i = 0; i < cl->count; ++i) {
        const gles_cmd *c = &cl->data[i];
        switch (c->type) {
        case GLES_CMD_COLOR4F:
            /* pass-through – bind vertex colour array */
            glEnableClientState(GL_COLOR_ARRAY);
            break;
        case GLES_CMD_TEX_ENV_COMBINE:
            /*
             * c->u[0] holds the desired texture env mode (GL_COMBINE).
             * c->u[1] selects the combine function, reused for both RGB
             * and ALPHA channels.
             */
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, c->u[0]);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, c->u[1]);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, c->u[1]);
            break;
        case GLES_CMD_MULTITEXCOORD4F:
            glClientActiveTexture(c->u[0]);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            break;
        case GLES_CMD_MATRIX_MODE:
            glMatrixMode(c->u[0]);
            /* glLoadMatrixf(MVP); — app supplies */
            break;
        case GLES_CMD_LOAD_CONSTANT:
            glColor4f(c->f[0], c->f[1], c->f[2], c->f[3]);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("usage: replay_runtime <shader.asm>");
        return 1;
    }
    GLES_CommandList cl;
    if (dx8gles11_compile_file(argv[1], NULL, &cl)) {
        fprintf(stderr, "%s\n", dx8gles11_error());
        return 1;
    }
    execute_cmds(&cl);
    gles_cmdlist_free(&cl);
    puts("OK");
    return 0;
}
