#ifndef DX8GLES11_H
#define DX8GLES11_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct GLES_CommandList; /* forward */

typedef struct dx8gles11_options {
    const char *include_dir; /* search path for #include */
    int optimize;            /* reserved */
} dx8gles11_options;

typedef enum gles_cmd_type {
    GLES_CMD_TEX_ENVF,
    GLES_CMD_TEX_ENV_COMBINE,
    GLES_CMD_COLOR4F,
    GLES_CMD_MULTITEXCOORD4F,
    GLES_CMD_VERTEX_ATTRIB,
    GLES_CMD_BIND_VBO,
    GLES_CMD_MATRIX_MODE,
    GLES_CMD_MATRIX_LOAD,
    GLES_CMD_TEX_MATRIX_MODE,
    GLES_CMD_TEX_MATRIX_LOAD,
    GLES_CMD_LOAD_IDENTITY,
    GLES_CMD_LIGHT_PARAM,
    GLES_CMD_LOAD_CONSTANT,
    GLES_CMD_TEX_SAMPLE,
    GLES_CMD_TEX_LOAD,
    GLES_CMD_TEX_COORD_COPY,
    GLES_CMD_TEX_KILL,
    GLES_CMD_TEX_IMAGE_2D,
    GLES_CMD_TEX_IMAGE_3D,
    GLES_CMD_TEX_IMAGE_DEPTH,
    /*
     * Emitted when the translator encounters an unsupported opcode or
     * invalid operand. For example, "mov oT8, r0" produces this command and
     * sets an error via dx8gles11_error().
     */
    GLES_CMD_UNKNOWN
} gles_cmd_type;

typedef struct gles_cmd {
    gles_cmd_type type;
    float f[4];
    uint32_t u[4];
} gles_cmd;

typedef struct GLES_CommandList {
    gles_cmd *data;
    size_t count;
    size_t capacity;
} GLES_CommandList;

/* API ----------------------------------------------------------- */
int dx8gles11_compile_file(const char *path, const dx8gles11_options *opts, GLES_CommandList *out);
int dx8gles11_compile_string(const char *src, const dx8gles11_options *opts, GLES_CommandList *out);
const char *dx8gles11_error(void);
void gles_cmdlist_free(GLES_CommandList *);
int dx8gles11_has_extension(const char *name);

#ifdef __cplusplus
}
#endif
#endif /* DX8GLES11_H */
