#include "minithread.h"
#include <stdlib.h>

struct mt_task {
  void (*func)(void *);
  void *arg;
  struct mt_task *next;
};

static int worker(void *arg) {
  mt_pool *p = arg;
  for (;;) {
    mtx_lock(&p->mtx);
    while (!p->head && !p->stop)
      cnd_wait(&p->cv, &p->mtx);
    if (p->stop && !p->head) {
      mtx_unlock(&p->mtx);
      break;
    }
    struct mt_task *t = p->head;
    p->head = t->next;
    if (!p->head)
      p->tail = NULL;
    p->active++;
    mtx_unlock(&p->mtx);

    t->func(t->arg);
    free(t);

    mtx_lock(&p->mtx);
    p->active--;
    if (!p->head && p->active == 0)
      cnd_broadcast(&p->cv);
    mtx_unlock(&p->mtx);
  }
  return 0;
}

int mt_pool_init(mt_pool *p, int n) {
  if (n <= 0)
    n = 1;
  p->threads = calloc((size_t)n, sizeof(thrd_t));
  if (!p->threads)
    return -1;
  p->num_threads = n;
  p->head = p->tail = NULL;
  p->stop = 0;
  p->active = 0;
  if (mtx_init(&p->mtx, mtx_plain) != thrd_success)
    return -1;
  if (cnd_init(&p->cv) != thrd_success)
    return -1;
  for (int i = 0; i < n; ++i)
    thrd_create(&p->threads[i], worker, p);
  return 0;
}

void mt_pool_destroy(mt_pool *p) {
  mtx_lock(&p->mtx);
  p->stop = 1;
  cnd_broadcast(&p->cv);
  mtx_unlock(&p->mtx);
  for (int i = 0; i < p->num_threads; ++i)
    thrd_join(p->threads[i], NULL);
  free(p->threads);
  mtx_destroy(&p->mtx);
  cnd_destroy(&p->cv);
}

int mt_pool_submit(mt_pool *p, void (*func)(void *), void *arg) {
  struct mt_task *t = malloc(sizeof(*t));
  if (!t)
    return -1;
  t->func = func;
  t->arg = arg;
  t->next = NULL;
  mtx_lock(&p->mtx);
  if (p->tail)
    p->tail->next = t;
  else
    p->head = t;
  p->tail = t;
  cnd_signal(&p->cv);
  mtx_unlock(&p->mtx);
  return 0;
}

void mt_pool_join(mt_pool *p) {
  mtx_lock(&p->mtx);
  while (p->head || p->active)
    cnd_wait(&p->cv, &p->mtx);
  mtx_unlock(&p->mtx);
}
