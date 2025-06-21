#ifndef DX8GLES11_UTILS_H
#define DX8GLES11_UTILS_H
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
static inline size_t *sb__raw(void *a) { return (size_t *)((char *)a - sizeof(size_t) * 2); }
#define sb_count(a) ((a) ? sb__raw(a)[0] : 0)
#define sb_capacity(a) ((a) ? sb__raw(a)[1] : 0)
#define sb_free(a)                                                                                 \
    do {                                                                                           \
        if (a) {                                                                                   \
            free(sb__raw(a));                                                                      \
            a = NULL;                                                                              \
        }                                                                                          \
    } while (0)
#define sb_push(a, v)                                                                              \
    do {                                                                                           \
        if (sb_count(a) == sb_capacity(a)) {                                                       \
            size_t newcap = sb_capacity(a) ? sb_capacity(a) * 2 : 8;                               \
            size_t *nb = (a) ? realloc(sb__raw(a), newcap * sizeof(*(a)) + sizeof(size_t) * 2)     \
                             : malloc(newcap * sizeof(*(a)) + sizeof(size_t) * 2);                 \
            if (!nb)                                                                               \
                abort();                                                                           \
            if (!a) {                                                                              \
                nb[0] = nb[1] = 0;                                                                 \
            }                                                                                      \
            a = (void *)((size_t *)nb + 2);                                                        \
            nb[1] = newcap;                                                                        \
        }                                                                                          \
        (a)[sb__raw(a)[0]++] = (v);                                                                \
    } while (0)

char *util_strndup(const char *s, size_t n);
char *util_strdup(const char *s);
int util_vasprintf(char **out, const char *fmt, va_list ap);
int util_asprintf(char **out, const char *fmt, ...);

#endif
