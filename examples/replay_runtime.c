#include "dx8gles11.h"
#include "runtime_pipeline.h"
#include <stdio.h>
#include <stdlib.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <string.h>

#ifndef GL_TEXTURE_3D_OES
#define GL_TEXTURE_3D_OES 0x806F
#endif
#ifndef GL_DEPTH_COMPONENT

#define GL_DEPTH_COMPONENT 0x1902
#endif

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


static void execute_cmds(const GLES_CommandList *cl) {
    for (size_t i = 0; i < cl->count; ++i) {
        const gles_cmd *c = &cl->data[i];
        switch (c->type) {
        case GLES_CMD_COLOR4F:
            /* pass-through – bind vertex colour array */
            glEnableClientState(GL_COLOR_ARRAY);
            break;
        case GLES_CMD_TEX_ENVF:
            /*
             * c->u[0] maps to the pname argument of glTexEnvf.
             * c->f[0] provides the float parameter value.
             */
            glTexEnvf(GL_TEXTURE_ENV, c->u[0], c->f[0]);
            break;
        case GLES_CMD_TEX_ENV_COMBINE:
            /*
             * c->u[0] holds the desired texture env mode (GL_COMBINE).
             * c->u[1] selects the combine function, reused for both RGB
             * and ALPHA channels.
             */
            if ((c->u[1] == GL_MAX_EXT || c->u[1] == GL_MIN_EXT) &&
                !dx8gles11_has_extension("GL_EXT_blend_minmax")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                return;
            }
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, c->u[0]);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, c->u[1]);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, c->u[1]);
            break;
        case GLES_CMD_MULTITEXCOORD4F:
            glClientActiveTexture(c->u[0]);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            break;
        case GLES_CMD_BIND_VBO:
            if (!dx8gles11_has_extension("GL_OES_vertex_buffer_object")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                return;
            }
            glBindBuffer(GL_ARRAY_BUFFER, c->u[0]);
            break;
        case GLES_CMD_VERTEX_ATTRIB:
            if (c->u[0] == 0) {
                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, 0, 0);
            } else if (c->u[0] == 1) {
                glEnableClientState(GL_COLOR_ARRAY);
                glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
            }
            break;
        case GLES_CMD_MATRIX_MODE:
            glMatrixMode(c->u[0]);
            /* glLoadMatrixf(MVP); — app supplies */
            break;
        case GLES_CMD_MATRIX_LOAD:
            glLoadMatrixf(c->f);
            break;
        case GLES_CMD_TEX_MATRIX_MODE:
            glActiveTexture(GL_TEXTURE0 + c->u[0]);
            glMatrixMode(GL_TEXTURE);
            break;
        case GLES_CMD_TEX_MATRIX_LOAD:
            glActiveTexture(GL_TEXTURE0 + c->u[0]);
            glLoadMatrixf(c->f);
            break;
        case GLES_CMD_LOAD_IDENTITY:
            glLoadIdentity();
            break;
        case GLES_CMD_LOAD_CONSTANT:
            glColor4f(c->f[0], c->f[1], c->f[2], c->f[3]);
            break;
        case GLES_CMD_TEX_IMAGE_2D:
            if (!dx8gles11_has_extension("GL_OES_texture_npot")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                return;
            }
            if (c->u[3])
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, c->u[2], c->u[0],
                                        c->u[1], 0, 0, NULL);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, c->u[2], c->u[0], c->u[1], 0,
                             c->u[2], GL_UNSIGNED_BYTE, NULL);
            break;
        case GLES_CMD_TEX_IMAGE_3D:
#ifdef GL_OES_texture_3D
            if (!dx8gles11_has_extension("GL_OES_texture_3D")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                return;
            }
            glTexImage3DOES(GL_TEXTURE_3D_OES, 0, c->u[3], c->u[0], c->u[1],
                            c->u[2], 0, c->u[3], GL_UNSIGNED_BYTE, NULL);
#endif
            break;
        case GLES_CMD_TEX_IMAGE_DEPTH:
            if (!dx8gles11_has_extension("GL_OES_depth_texture")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                return;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, c->u[0], c->u[1],
                         0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        puts("usage: replay_runtime <shader.asm> [threads]");
        return 1;
    }
    int threads = 2;
    if (argc > 2)
        threads = atoi(argv[2]);

    char *src = read_file(argv[1]);
    if (!src) {
        fprintf(stderr, "failed to read %s\n", argv[1]);
        return 1;
    }

    pipeline p;
    if (pipeline_init(&p, threads) || pipeline_start(&p)) {
        fprintf(stderr, "pipeline init failed\n");
        free(src);
        return 1;
    }

    lf_queue_push(&p.decode_q, src);
    pipeline_stop(&p);
    double cps = pipeline_commands_per_second(&p);
    pipeline_join(&p);
    free(src);

    printf("OK (%.2f cmds/s)\n", cps);
    return 0;
}
