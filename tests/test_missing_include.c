#include "preprocess.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    char *err = NULL;
    char *out = pp_run("fixtures/missing_inc.asm", NULL, &err);
    if (out != NULL) {
        fprintf(stderr, "pp_run unexpectedly succeeded\n");
        free(out);
        free(err);
        return 1;
    }
    if (!err || strstr(err, "include") == NULL) {
        fprintf(stderr, "bad error: %s\n", err ? err : "(null)");
        free(err);
        return 1;
    }
    free(err);
    return 0;
}
