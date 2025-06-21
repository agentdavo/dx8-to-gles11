#include "dx8gles11.h"
#include "minithread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

static void compile_task(void *arg) {
  const char *src = arg;
  GLES_CommandList cl;
  dx8gles11_compile_string(src, NULL, &cl);
  gles_cmdlist_free(&cl);
}

static double bench_serial(const char *src, int iters) {
  struct timespec s, e;
  clock_gettime(CLOCK_MONOTONIC, &s);
  for (int i = 0; i < iters; ++i)
    compile_task((void *)src);
  clock_gettime(CLOCK_MONOTONIC, &e);
  return (e.tv_sec - s.tv_sec) * 1000.0 + (e.tv_nsec - s.tv_nsec) / 1e6;
}

static double bench_threaded(const char *src, int iters, int threads) {
  mt_pool p;
  mt_pool_init(&p, threads);
  struct timespec s, e;
  clock_gettime(CLOCK_MONOTONIC, &s);
  for (int i = 0; i < iters; ++i)
    mt_pool_submit(&p, compile_task, (void *)src);
  mt_pool_join(&p);
  clock_gettime(CLOCK_MONOTONIC, &e);
  mt_pool_destroy(&p);
  return (e.tv_sec - s.tv_sec) * 1000.0 + (e.tv_nsec - s.tv_nsec) / 1e6;
}

int main(int argc, char **argv) {
  const char *path = argc > 1 ? argv[1] : "../tests/fixtures/matrix_ops.asm";
  int iters = argc > 2 ? atoi(argv[2]) : 100;
  int threads = argc > 3 ? atoi(argv[3]) : 4;
  char *src = read_file(path);
  if (!src) {
    fprintf(stderr, "failed to read %s\n", path);
    return 1;
  }
  double t_serial = bench_serial(src, iters);
  double t_thread = bench_threaded(src, iters, threads);
  printf(
      "Iterations: %d\nSerial time: %.2f ms\nThreaded (%d threads): %.2f ms\n",
      iters, t_serial, threads, t_thread);
  free(src);
  return 0;
}
