#include "runtime_pipeline.h"
#include "dx8asm_parser.h"
#include "dx8gles11.h"
#include "utils.h"
#include <threads.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* translator provided by dx8_to_gles11.c */
extern void translate_instr(const asm_instr *restrict, GLES_CommandList *restrict);

static _Thread_local char g_err[256] = "";
/* per-thread counter tracking active pipeline starts */
static _Thread_local unsigned g_started = 0;

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
    if (lf_queue_init(&p->jobs)) {
        set_err("queue init failed");
        return -1;
    }
    p->num_threads = num_threads > 0 ? num_threads : 1;
    if (mt_pool_init(&p->workers, p->num_threads)) {
        set_err("thread pool init failed");
        lf_queue_destroy(&p->jobs);
        return -1;
    }
    return 0;
}

int pipeline_start(pipeline *p) {
    (void)p;
    g_started++;
    return 0;
}

void pipeline_stop(pipeline *p) {
    (void)p;
    if (g_started)
        g_started--;
}

void pipeline_join(pipeline *p) {
    mt_pool_join(&p->workers);
    mt_pool_destroy(&p->workers);
    lf_queue_destroy(&p->jobs);
}
