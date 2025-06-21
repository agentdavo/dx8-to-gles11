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

    const char *ps13_src =
        "ps.1.3\n"
        "tex t0\n"
        "tex t1\n"
        "tex t2\n"
        "tex t3\n"
        "nop\n"  /* 1 */
        "nop\n"  /* 2 */
        "nop\n"  /* 3 */
        "nop\n"  /* 4 */
        "nop\n"  /* 5 */
        "nop\n"  /* 6 */
        "nop\n"  /* 7 */
        "nop\n"  /* 8 */
        "nop\n"  /* 9 */
        "nop\n"  /*10*/
        "nop\n"  /*11*/
        "nop\n"  /*12*/
        "nop\n"; /*13th arithmetic*/
    if (dx8gles11_compile_string(ps13_src, NULL, &cl) == 0) {
        fprintf(stderr, "ps1.3 limit not enforced\n");
        gles_cmdlist_free(&cl);
        return 1;
    }
    if (!strstr(dx8gles11_error(), "ps.1.3")) {
        fprintf(stderr, "bad ps1.3 error: %s\n", dx8gles11_error());
        return 1;
    }

    const char *ps13_arith_only =
        "ps.1.3\n"
        "nop\n" /* 1 */
        "nop\n" /* 2 */
        "nop\n" /* 3 */
        "nop\n" /* 4 */
        "nop\n" /* 5 */
        "nop\n" /* 6 */
        "nop\n" /* 7 */
        "nop\n" /* 8 */
        "nop\n" /* 9 */
        "nop\n" /*10*/
        "nop\n" /*11*/
        "nop\n" /*12*/
        "nop\n"; /*13th arithmetic*/
    if (dx8gles11_compile_string(ps13_arith_only, NULL, &cl) == 0) {
        fprintf(stderr, "ps1.3 arith-only limit not enforced\n");
        gles_cmdlist_free(&cl);
        return 1;
    }
    if (!strstr(dx8gles11_error(), "ps.1.3")) {
        fprintf(stderr, "bad ps1.3 arith-only error: %s\n", dx8gles11_error());
        return 1;
    }
    return 0;
}
