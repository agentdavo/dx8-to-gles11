#ifndef DX8GLES11_MINITHREAD_H
#define DX8GLES11_MINITHREAD_H

#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mt_task;
typedef struct mt_pool {
  thrd_t *threads;
  int num_threads;
  mtx_t mtx;
  cnd_t cv;
  struct mt_task *head;
  struct mt_task *tail;
  int stop;
  int active;
} mt_pool;

int mt_pool_init(mt_pool *p, int num_threads);
void mt_pool_destroy(mt_pool *p);
int mt_pool_submit(mt_pool *p, void (*func)(void *), void *arg);
void mt_pool_join(mt_pool *p);

int mt_thread_start(thrd_t *t, void (*func)(void *), void *arg);
int mt_thread_join(thrd_t t);

#ifdef __cplusplus
}
#endif

#endif /* DX8GLES11_MINITHREAD_H */
