#include "preprocess.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int fail_after = 0;
void set_fail_after(int n) { fail_after = n; }

void *__real_realloc(void *ptr, size_t size);
void *__wrap_realloc(void *ptr, size_t size) {
    if (fail_after > 0) {
        fail_after--;
        if (fail_after == 0)
            return NULL;
    }
    return __real_realloc(ptr, size);
}

int main(void) {
    set_fail_after(1); /* fail first realloc */
    char *err = NULL;
    char *out = pp_run(ASM_PATH, NULL, &err);
    if (out != NULL) {
        fprintf(stderr, "pp_run unexpectedly succeeded\n");
        free(out);
        free(err);
        return 1;
    }
    if (!err || strstr(err, "memory") == NULL) {
        fprintf(stderr, "bad error: %s\n", err ? err : "(null)");
        free(err);
        return 1;
    }
    free(err);
    return 0;
}
