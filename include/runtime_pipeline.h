#ifndef DX8GLES11_RUNTIME_PIPELINE_H
#define DX8GLES11_RUNTIME_PIPELINE_H

#include "minithread.h"
#include "lf_queue.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Simple runtime pipeline for executing GLES command lists.
 * Typical applications create 2-4 worker threads.
 */

struct pipeline_stats;

typedef struct pipeline {
    mt_pool workers;
    lf_queue decode_q;
    lf_queue prepare_q;
    lf_queue dispatch_q;
    int decode_threads;
    int prepare_threads;
    int num_threads; /* dispatch threads */
    struct pipeline_stats *stats;
} pipeline;

double pipeline_commands_per_second(const pipeline *p);

int pipeline_init(pipeline *p, int num_threads);
int pipeline_init_stages(pipeline *p, int decode_threads, int prepare_threads,
                         int dispatch_threads);
int pipeline_start(pipeline *p);
void pipeline_stop(pipeline *p);
void pipeline_join(pipeline *p);

static inline double ts_diff(const struct timespec *restrict s,
                             const struct timespec *restrict e) {
    return (e->tv_sec - s->tv_sec) + (e->tv_nsec - s->tv_nsec) / 1e9;
}

#define TS_DIFF(s, e)                                                         \
    _Generic((s), struct timespec *: ts_diff, const struct timespec *: ts_diff)( \
        (s), (e))

#ifdef __cplusplus
}
#endif

#endif /* DX8GLES11_RUNTIME_PIPELINE_H */
