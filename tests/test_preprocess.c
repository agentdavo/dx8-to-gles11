#include "preprocess.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    const char *path = "tmp_test.asm";
    FILE *f = fopen(path, "w");
    if (!f)
        return 1;
    fputs("#define FOO 1\nFOO\nFOOBAR\n", f);
    fclose(f);
    char *err = NULL;
    char *out = pp_run(path, ".", &err);
    remove(path);
    if (!out) {
        fprintf(stderr, "pp_run failed: %s\n", err ? err : "unknown");
        free(err);
        return 1;
    }
    int ok = strcmp(out, "1\nFOOBAR\n") == 0;
    if (!ok)
        fprintf(stderr, "Unexpected output: '%s'\n", out);
    free(out);
    free(err);
    return ok ? 0 : 1;
}
