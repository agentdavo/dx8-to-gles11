#include "dx8gles11.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *ps_src =
        "ps.1.1\n"
        "tex t0\n"
        "tex t1\n"
        "tex t2\n"
        "tex t3\n"
        "tex t0\n"; /* fifth tex */
    GLES_CommandList cl;
    if (dx8gles11_compile_string(ps_src, NULL, &cl) == 0) {
        fprintf(stderr, "ps limit not enforced\n");
        gles_cmdlist_free(&cl);
        return 1;
    }
    if (!strstr(dx8gles11_error(), "ps.1.1")) {
        fprintf(stderr, "bad ps error: %s\n", dx8gles11_error());
        return 1;
    }

    const char *vs_src =
        "vs.1.1\n"
        "def c0, 0,0,0,0\n"
        "def c1, 0,0,0,0\n"
        "def c2, 0,0,0,0\n"
        "def c3, 0,0,0,0\n"
        "def c4, 0,0,0,0\n"
        "def c5, 0,0,0,0\n"
        "def c6, 0,0,0,0\n"
        "def c7, 0,0,0,0\n"
        "def c8, 0,0,0,0\n"
        "def c9, 0,0,0,0\n"
        "def c10, 0,0,0,0\n"
        "def c11, 0,0,0,0\n"
        "def c12, 0,0,0,0\n" /* 13th constant */
        "mov oPos, v0\n";
    if (dx8gles11_compile_string(vs_src, NULL, &cl) == 0) {
        fprintf(stderr, "vs limit not enforced\n");
        gles_cmdlist_free(&cl);
        return 1;
    }
    if (!strstr(dx8gles11_error(), "vs.1.1")) {
        fprintf(stderr, "bad vs error: %s\n", dx8gles11_error());
        return 1;
    }
    return 0;
}
