#include "preprocess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *err = NULL;
    char *out = pp_run("fixtures/root.asm", NULL, &err);
    if (!out) {
        fprintf(stderr, "%s\n", err ? err : "pp_run failed");
        free(err);
        return 1;
    }
    const char *expect = "foo\nbar\nbaz\n";
    if (strcmp(out, expect) != 0) {
        fprintf(stderr, "unexpected:\n%s\n", out);
        free(out);
        return 1;
    }
    free(out);
    return 0;
}
