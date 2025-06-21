#include "lf_queue.h"
#include <stdlib.h>

int lf_queue_init(lf_queue *q) {
    lf_queue_node *n = malloc(sizeof(*n));
    if (!n)
        return -1;
    n->next = NULL;
    n->value = NULL;
    atomic_store(&q->head, n);
    atomic_store(&q->tail, n);
    return 0;
}

void lf_queue_destroy(lf_queue *q) {
    lf_queue_node *n = atomic_load(&q->head);
    while (n) {
        lf_queue_node *next = n->next;
        free(n);
        n = next;
    }
}

int lf_queue_push(lf_queue *q, void *value) {
    lf_queue_node *n = malloc(sizeof(*n));
    if (!n)
        return -1;
    n->value = value;
    n->next = NULL;

    for (;;) {
        lf_queue_node *tail = atomic_load_explicit(&q->tail, memory_order_acquire);
        lf_queue_node *next = atomic_load_explicit(&tail->next, memory_order_acquire);
        if (next == NULL) {
            if (atomic_compare_exchange_weak_explicit(&tail->next, &next, n,
                                                      memory_order_release,
                                                      memory_order_relaxed)) {
                atomic_compare_exchange_weak_explicit(&q->tail, &tail, n,
                                                      memory_order_release,
                                                      memory_order_relaxed);
                return 0;
            }
        } else {
            atomic_compare_exchange_weak_explicit(&q->tail, &tail, next,
                                                  memory_order_release,
                                                  memory_order_relaxed);
        }
    }
}

void *lf_queue_pop(lf_queue *q) {
    for (;;) {
        lf_queue_node *head = atomic_load_explicit(&q->head, memory_order_acquire);
        lf_queue_node *tail = atomic_load_explicit(&q->tail, memory_order_acquire);
        lf_queue_node *next = atomic_load_explicit(&head->next, memory_order_acquire);
        if (head == tail) {
            if (next == NULL)
                return NULL;
            atomic_compare_exchange_weak_explicit(&q->tail, &tail, next,
                                                  memory_order_release,
                                                  memory_order_relaxed);
        } else {
            void *value = next->value;
            if (atomic_compare_exchange_weak_explicit(&q->head, &head, next,
                                                      memory_order_release,
                                                      memory_order_relaxed)) {
                free(head);
                return value;
            }
        }
    }
}

