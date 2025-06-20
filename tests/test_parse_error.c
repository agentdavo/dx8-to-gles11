#include "dx8gles11.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "need path\n");
        return 1;
    }
    GLES_CommandList cl;
    if (dx8gles11_compile_file(argv[1], NULL, &cl) == 0) {
        fprintf(stderr, "unexpected success\n");
        gles_cmdlist_free(&cl);
        return 1;
    }
    const char *e = dx8gles11_error();
    if (!e || strstr(e, "invalid") == NULL) {
        fprintf(stderr, "bad message: %s\n", e ? e : "(null)");
        return 1;
    }
    return 0;
}
