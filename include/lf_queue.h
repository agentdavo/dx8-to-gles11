#ifndef DX8GLES11_LF_QUEUE_H
#define DX8GLES11_LF_QUEUE_H

#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lf_queue_node {
    void *value;
    struct lf_queue_node *next;
} lf_queue_node;

typedef struct lf_queue {
    _Atomic(lf_queue_node *) head;
    _Atomic(lf_queue_node *) tail;
} lf_queue;

int lf_queue_init(lf_queue *q);
void lf_queue_destroy(lf_queue *q);
int lf_queue_push(lf_queue *q, void *value);
void *lf_queue_pop(lf_queue *q);

#ifdef __cplusplus
}
#endif

#endif /* DX8GLES11_LF_QUEUE_H */
