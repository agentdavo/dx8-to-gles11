#include "dx8gles11.h"
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

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : "../tests/fixtures";
    int iters = argc > 2 ? atoi(argv[2]) : 100000;

    const char *shaders[] = {
        "mov_tex",
        "mul_const",
        "dp3_matrix",
        "add",
        "matrix_ops",
        "tex_ops",
        "terrain_ps",
        "motion_blur_vs",
        "river_water_ps",
        "water_reflection_ps",
        "water_trapezoid_ps",
        "max_min",
        "cnd",
        "nop",
        "ps13_ops",
        "tex_matrix"
    };
    const size_t num = sizeof(shaders) / sizeof(shaders[0]);

    size_t total_cmds = 0;
    double total_time = 0.0;

    for (size_t i = 0; i < num; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s.asm", dir, shaders[i]);
        char *src = read_file(path);
        if (!src) {
            fprintf(stderr, "failed to read %s\n", path);
            continue;
        }
        struct timespec s, e;
        clock_gettime(CLOCK_MONOTONIC, &s);
        size_t cmds = 0;
        for (int j = 0; j < iters; ++j) {
            GLES_CommandList cl;
            if (dx8gles11_compile_string(src, NULL, &cl) == 0) {
                cmds += cl.count;
                gles_cmdlist_free(&cl);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &e);
        double elapsed = (e.tv_sec - s.tv_sec) +
                         (e.tv_nsec - s.tv_nsec) / 1e9;
        double cps = elapsed > 0.0 ? cmds / elapsed : 0.0;
        printf("%s: %.2f cmds/s\n", shaders[i], cps);
        total_cmds += cmds;
        total_time += elapsed;
        free(src);
    }
    if (total_time > 0.0)
        printf("Overall: %.2f cmds/s\n", total_cmds / total_time);
    return 0;
}
