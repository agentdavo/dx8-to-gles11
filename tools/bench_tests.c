#include "dx8gles11.h"
#include "runtime_pipeline.h"
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
    int stage1 = 1, stage2 = 1, stage3 = 2;
    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "-stage1") == 0 && i + 1 < argc) {
            stage1 = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-stage2") == 0 && i + 1 < argc) {
            stage2 = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-stage3") == 0 && i + 1 < argc) {
            stage3 = atoi(argv[++i]);
        }
    }

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
        GLES_CommandList tmp;
        if (dx8gles11_compile_string(src, NULL, &tmp) != 0) {
            fprintf(stderr, "compile failed: %s\n", dx8gles11_error());
            free(src);
            continue;
        }
        size_t cmd_count = tmp.count;
        gles_cmdlist_free(&tmp);

        struct timespec s, e;
        pipeline p;
        clock_gettime(CLOCK_MONOTONIC, &s);
        if (pipeline_init_stages(&p, stage1, stage2, stage3) || pipeline_start(&p)) {
            fprintf(stderr, "pipeline init failed\n");
            free(src);
            continue;
        }
        for (int j = 0; j < iters; ++j)
            lf_queue_push(&p.decode_q, src);
        pipeline_stop(&p);
        double cps = pipeline_commands_per_second(&p);
        pipeline_join(&p);
        clock_gettime(CLOCK_MONOTONIC, &e);
        double elapsed = TS_DIFF(&s, &e);
        size_t cmds = cmd_count * (size_t)iters;
        if (elapsed > 0.0)
            cps = cmds / elapsed;
        printf("%s: %.2f cmds/s\n", shaders[i], cps);
        total_cmds += cmds;
        total_time += elapsed;
        free(src);
    }
    if (total_time > 0.0)
        printf("Overall: %.2f cmds/s\n", total_cmds / total_time);
    return 0;
}
