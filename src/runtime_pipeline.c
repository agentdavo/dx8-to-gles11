#include "runtime_pipeline.h"
#include "dx8asm_parser.h"
#include "dx8gles11.h"
#include "utils.h"
#include <threads.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#ifndef GL_TEXTURE_3D_OES
#define GL_TEXTURE_3D_OES 0x806F
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif

/* translator provided by dx8_to_gles11.c */
extern void translate_instr(const asm_instr *restrict, GLES_CommandList *restrict);

static _Thread_local char g_err[256] = "";
/* per-thread counter tracking active pipeline starts */
static _Thread_local unsigned g_started = 0;

#define PIPELINE_MIN_THREADS 1
#define PIPELINE_MAX_THREADS 8
_Static_assert(PIPELINE_MAX_THREADS >= PIPELINE_MIN_THREADS,
               "invalid pipeline thread range");

typedef struct pipeline_stats {
    struct timespec start;
    size_t commands;
} pipeline_stats;

typedef struct decode_ctx {
    lf_queue *decode_q;
    lf_queue *prepare_q;
    asm_instr *buffer;
} decode_ctx;

typedef struct prepare_ctx {
    lf_queue *prepare_q;
    lf_queue *dispatch_q;
    gles_cmd *buffer;
} prepare_ctx;

typedef struct dispatch_ctx {
    lf_queue *dispatch_q;
    pipeline_stats *stats;
} dispatch_ctx;

static void decode_worker(void *arg) {
    decode_ctx *restrict ctx = arg;
    for (;;) {
        char *src = lf_queue_pop(ctx->decode_q);
        if (!src) {
            if (!g_started)
                break;
            thrd_yield();
            continue;
        }

        asm_program prog = {0};
        char *err = NULL;
        if (asm_parse(src, &prog, &err) == 0) {
            for (size_t i = 0; i < prog.count; ++i) {
                sb_push(ctx->buffer, prog.code[i]);
                asm_instr *in = &ctx->buffer[sb_count(ctx->buffer) - 1];
                lf_queue_push(ctx->prepare_q, in);
            }
        }
        free(err);
        asm_program_free(&prog);
    }
    sb_free(ctx->buffer);
    free(ctx);
}

static void prepare_worker(void *arg) {
    prepare_ctx *restrict ctx = arg;
    GLES_CommandList list = {0};
    for (;;) {
        asm_instr *in = lf_queue_pop(ctx->prepare_q);
        if (!in) {
            if (!g_started)
                break;
            thrd_yield();
            continue;
        }
        translate_instr(in, &list);
        for (size_t i = 0; i < list.count; ++i) {
            sb_push(ctx->buffer, list.data[i]);
            gles_cmd *out = &ctx->buffer[sb_count(ctx->buffer) - 1];
            lf_queue_push(ctx->dispatch_q, out);
        }
        gles_cmdlist_free(&list);
    }
    sb_free(ctx->buffer);
    free(ctx);
}

static void dispatch_worker(void *arg) {
    dispatch_ctx *restrict ctx = arg;
    pipeline_stats *s = ctx->stats;
    memset(s, 0, sizeof(*s));
    timespec_get(&s->start, TIME_UTC);

    for (;;) {
        gles_cmd *c = lf_queue_pop(ctx->dispatch_q);
        if (!c) {
            if (!g_started)
                break;
            thrd_yield();
            continue;
        }

        switch (c->type) {
        case GLES_CMD_COLOR4F:
            glEnableClientState(GL_COLOR_ARRAY);
            break;
        case GLES_CMD_TEX_ENVF:
            glTexEnvf(GL_TEXTURE_ENV, c->u[0], c->f[0]);
            break;
        case GLES_CMD_TEX_ENV_COMBINE:
            if ((c->u[1] == GL_MAX_EXT || c->u[1] == GL_MIN_EXT) &&
                !dx8gles11_has_extension("GL_EXT_blend_minmax")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                break;
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
                break;
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
                break;
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
                break;
            }
            glTexImage3DOES(GL_TEXTURE_3D_OES, 0, c->u[3], c->u[0], c->u[1],
                            c->u[2], 0, c->u[3], GL_UNSIGNED_BYTE, NULL);
#endif
            break;
        case GLES_CMD_TEX_IMAGE_DEPTH:
            if (!dx8gles11_has_extension("GL_OES_depth_texture")) {
                fprintf(stderr, "%s\n", dx8gles11_error());
                break;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, c->u[0], c->u[1],
                         0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
            break;
        default:
            break;
        }

        s->commands++;
    }
    free(ctx);
}

static void set_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err, sizeof(g_err), fmt, ap);
    va_end(ap);
}

/* initialize a pipeline; applications typically use 2-4 threads */
int pipeline_init(pipeline *p, int num_threads) {
    if (!p)
        return -1;
    if (lf_queue_init(&p->decode_q) || lf_queue_init(&p->prepare_q) ||
        lf_queue_init(&p->dispatch_q)) {
        set_err("queue init failed");
        lf_queue_destroy(&p->decode_q);
        lf_queue_destroy(&p->prepare_q);
        lf_queue_destroy(&p->dispatch_q);
        return -1;
    }
    if (num_threads < PIPELINE_MIN_THREADS)
        num_threads = PIPELINE_MIN_THREADS;
    if (num_threads > PIPELINE_MAX_THREADS)
        num_threads = PIPELINE_MAX_THREADS;
    p->num_threads = num_threads;
    p->stats = calloc((size_t)p->num_threads, sizeof(*p->stats));
    if (!p->stats) {
        set_err("stats alloc failed");
        lf_queue_destroy(&p->decode_q);
        lf_queue_destroy(&p->prepare_q);
        lf_queue_destroy(&p->dispatch_q);
        return -1;
    }
    if (mt_pool_init(&p->workers, p->num_threads)) {
        set_err("thread pool init failed");
        free(p->stats);
        lf_queue_destroy(&p->decode_q);
        lf_queue_destroy(&p->prepare_q);
        lf_queue_destroy(&p->dispatch_q);
        return -1;
    }
    return 0;
}

int pipeline_start(pipeline *p) {
    if (!p)
        return -1;

    decode_ctx *dctx = calloc(1, sizeof(*dctx));
    prepare_ctx *pct = calloc(1, sizeof(*pct));
    if (!dctx || !pct) {
        free(dctx);
        free(pct);
        set_err("ctx alloc failed");
        return -1;
    }

    dispatch_ctx **dispatch = calloc((size_t)p->num_threads, sizeof(*dispatch));
    if (!dispatch) {
        free(dctx);
        free(pct);
        set_err("ctx alloc failed");
        return -1;
    }

    dctx->decode_q = &p->decode_q;
    dctx->prepare_q = &p->prepare_q;
    pct->prepare_q = &p->prepare_q;
    pct->dispatch_q = &p->dispatch_q;

    if (mt_pool_submit(&p->workers, decode_worker, dctx) ||
        mt_pool_submit(&p->workers, prepare_worker, pct)) {
        free(dctx);
        free(pct);
        free(dispatch);
        set_err("submit failed");
        return -1;
    }

    for (int i = 0; i < p->num_threads; ++i) {
        dispatch[i] = calloc(1, sizeof(**dispatch));
        if (!dispatch[i]) {
            set_err("ctx alloc failed");
            return -1;
        }
        dispatch[i]->dispatch_q = &p->dispatch_q;
        dispatch[i]->stats = &p->stats[i];
        if (mt_pool_submit(&p->workers, dispatch_worker, dispatch[i])) {
            set_err("submit failed");
            return -1;
        }
    }

    free(dispatch);
    g_started++;
    return 0;
}

void pipeline_stop(pipeline *p) {
    if (!p)
        return;
    if (g_started) {
        g_started--;
        mt_pool_join(&p->workers);
    }
}

void pipeline_join(pipeline *p) {
    mt_pool_join(&p->workers);
    mt_pool_destroy(&p->workers);
    lf_queue_destroy(&p->decode_q);
    lf_queue_destroy(&p->prepare_q);
    lf_queue_destroy(&p->dispatch_q);
    free(p->stats);
}

double pipeline_commands_per_second(const pipeline *p) {
    if (!p || !p->stats)
        return 0.0;
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    double cps = 0.0;
    for (int i = 0; i < p->num_threads; ++i) {
        const pipeline_stats *s = &p->stats[i];
        double elapsed = (now.tv_sec - s->start.tv_sec) +
                         (now.tv_nsec - s->start.tv_nsec) / 1e9;
        if (elapsed > 0.0)
            cps += s->commands / elapsed;
    }
    return cps;
}
