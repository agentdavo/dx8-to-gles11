#include "dx8gles11.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "bad.asm";
    FILE *f = fopen(path, "w");
    if (!f)
        return 1;
    fputs("; comment only\n", f);
    fclose(f);
    GLES_CommandList cl;
    int r = dx8gles11_compile_file(path, NULL, &cl);
    remove(path);
    if (r == 0) {
        gles_cmdlist_free(&cl);
        fprintf(stderr, "expected failure\n");
        return 1;
    }
    const char *err = dx8gles11_error();
    if (!err || strstr(err, "could not parse") == NULL) {
        fprintf(stderr, "bad error: %s\n", err ? err : "(null)");
        return 1;
    }
    return 0;
}
