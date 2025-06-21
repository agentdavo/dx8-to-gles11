#include "runtime_pipeline.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Thread_local char g_err[256] = "";
/* per-thread counter tracking active pipeline starts */
static _Thread_local unsigned g_started = 0;

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
