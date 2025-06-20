#include "preprocess.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct macro {
    char *name;
    char *value;
} macro;
static void macros_free(macro *m) {
    for (size_t i = 0; i < sb_count(m); ++i) {
        free(m[i].name);
        free(m[i].value);
    }
    sb_free(m);
}
static const char *skip_ws(const char *s) {
    while (*s && isspace((unsigned char)*s))
        ++s;
    return s;
}
static char *read_file(const char *p) {
    FILE *f = fopen(p, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *b = malloc(sz + 1);
    fread(b, 1, sz, f);
    fclose(f);
    b[sz] = '\0';
    return b;
}
static char *subst_macros(const char *line, macro *macros) {
    char *out = strdup(line);
    for (size_t i = 0; i < sb_count(macros); ++i) {
        char *pos;
        while ((pos = strstr(out, macros[i].name))) {
            size_t newlen = strlen(out) - strlen(macros[i].name) + strlen(macros[i].value);
            char *tmp = malloc(newlen + 1);
            size_t pref = pos - out;
            memcpy(tmp, out, pref);
            strcpy(tmp + pref, macros[i].value);
            strcpy(tmp + pref + strlen(macros[i].value), pos + strlen(macros[i].name));
            free(out);
            out = tmp;
        }
    }
    return out;
}
static char *process(const char *src_path, const char *src, const char *inc_dir, macro *macros,
                     char **err) {
    (void)src_path;
    char *out = NULL;
    size_t out_len = 0;
    const char *cur = src;
    while (*cur) {
        const char *ls = cur;
        while (*cur && *cur != '\n')
            ++cur;
        size_t ll = cur - ls;
        if (*cur == '\n')
            ++cur;
        char *line = util_strndup(ls, ll);
        const char *trim = skip_ws(line);
        if (*trim == '#') {
            trim++;
            if (!strncmp(trim, "include", 7)) {
                trim += 7;
                trim = skip_ws(trim);
                if (*trim == '\"') {
                    const char *p = ++trim;
                    while (*trim && *trim != '\"')
                        ++trim;
                    char *inc_name = util_strndup(p, trim - p);
                    char path[260];
                    snprintf(path, sizeof(path), "%s/%s", inc_dir ? inc_dir : ".", inc_name);
                    char *inc_src = read_file(path);
                    if (!inc_src) {
                        util_asprintf(err, "Could not open include '%s'", inc_name);
                        free(inc_name);
                        free(line);
                        continue;
                    }
                    char *inc_out = process(path, inc_src, inc_dir, macros, err);
                    if (!inc_out) {
                        free(inc_src);
                        free(inc_name);
                        free(line);
                        return NULL;
                    }
                    size_t inc_len = strlen(inc_out);
                    out = realloc(out, out_len + inc_len + 1);
                    memcpy(out + out_len, inc_out, inc_len);
                    out_len += inc_len;
                    out[out_len] = '\0';
                    free(inc_src);
                    free(inc_out);
                    free(inc_name);
                }
            } else if (!strncmp(trim, "define", 6)) {
                trim += 6;
                trim = skip_ws(trim);
                const char *ns = trim;
                while (*trim && !isspace((unsigned char)*trim))
                    ++trim;
                char *name = util_strndup(ns, trim - ns);
                trim = skip_ws(trim);
                char *val = strdup(trim);
                macro m = {name, val};
                sb_push(macros, m);
            }
        } else {
            char *exp = subst_macros(line, macros);
            size_t l = strlen(exp);
            out = realloc(out, out_len + l + 2);
            memcpy(out + out_len, exp, l);
            out_len += l;
            out[out_len++] = '\n';
            out[out_len] = '\0';
            free(exp);
        }
        free(line);
    }
    return out;
}
char *pp_run(const char *src_p, const char *inc_dir, char **err) {
    char *s = read_file(src_p);
    if (!s) {
        *err = strdup("Could not read source file");
        return NULL;
    }
    macro *macros = NULL;
    char *o = process(src_p, s, inc_dir, macros, err);
    macros_free(macros);
    free(s);
    return o;
}
