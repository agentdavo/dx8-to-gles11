#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *util_strndup(const char *s, size_t n) {
    size_t len = 0;
    while (len < n && s[len])
        ++len;
    char *out = malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

char *util_strdup(const char *s) { return util_strndup(s, strlen(s)); }

int util_vasprintf(char **out, const char *fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int len = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (len < 0) {
        *out = NULL;
        return -1;
    }
    *out = malloc((size_t)len + 1);
    if (!*out)
        return -1;
    return vsnprintf(*out, (size_t)len + 1, fmt, ap);
}

int util_asprintf(char **out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = util_vasprintf(out, fmt, ap);
    va_end(ap);
    return r;
}
