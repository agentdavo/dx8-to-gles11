#ifndef DX8GLES11_PREPROCESS_H
#define DX8GLES11_PREPROCESS_H
char *pp_run(const char *source_path, const char *include_dir, char **err);
char *pp_run_string(const char *source, const char *include_dir, char **err);
#endif
