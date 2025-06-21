#ifndef DX8GLES11_RUNTIME_PIPELINE_H
#define DX8GLES11_RUNTIME_PIPELINE_H

#include "minithread.h"
#include "lf_queue.h"

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
    int num_threads;
    struct pipeline_stats *stats;
} pipeline;

double pipeline_commands_per_second(const pipeline *p);

int pipeline_init(pipeline *p, int num_threads);
int pipeline_start(pipeline *p);
void pipeline_stop(pipeline *p);
void pipeline_join(pipeline *p);

#ifdef __cplusplus
}
#endif

#endif /* DX8GLES11_RUNTIME_PIPELINE_H */
