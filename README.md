# File: README.md

---

# DX8 → GLES 1.1 Translator

A **tiny C‑11 static library** that ingests legacy **DirectX 8 shaders**, runs a minimal **C‑style pre‑processor** (`#include`, `#define`), parses the DX8 opcodes, and emits an **in‑memory command list** representing equivalent **OpenGL ES 1.1 fixed‑function** state calls.

> **Why?**  
> Early 2000‑era games often shipped vertex shaders authored for DirectX 8 GPUs (GeForce 3/4, Radeon 8500). On embedded or retro devices you may only have GLES 1.x. This project lets you replay those shaders—colour modulation, texture combiners, MVP transforms—on vintage or embedded hardware **without** baking new GLSL.

---

## Features

| Area                         | Support | Notes |
|------------------------------|---------|-------|
| `#include` / `#define`       | ✅      | No macro parameters yet. |
| DX8 opcodes → IR             | ✅      | `mov`, `dp4`, `mul`, `mad`, more easy to add. |
| Opcode → GLES combiner       | ✅      | Maps core `GL_COMBINE`, `GL_MODULATE`, `GL_ADD_SIGNED`, etc. |
| Matrix load / MVP            | ✅      | Emits `GLES_CMD_MATRIX_MODE` + runtime load. |
| Multi‑texture coords         | ✅      | `mov oTn, …` → `glClientActiveTexture`. |
| VBO support (OES)            | ✅      | Emits `GLES_CMD_BIND_VBO` when available. |
| Error API                    | ✅      | `dx8gles11_error()` returns last human string. |
| Shader profile validation    | ✅      | Exceeding ps.1.1, ps.1.3 or vs.1.1 limits fails compilation. |
| Command‑list heap            | ✅      | Stretchy‑buffer; no external deps. |
| Build system                 | ✅      | Portable **CMake ≥ 3.16**. |
| Optimisation pass            | ⬜️      | Planned (dead code / constant folding). |
| Fragment shaders             | ⬜️      | Not covered—GLES 1.1 has none (consider IMG/ARB extensions). |

* Additional `ps.1.3` instructions are recognised—`cnd` and texture dot-product ops (`texdp3`, `texdp3tex`, `texm3x3`).
* Loading NPOT, volume and depth textures requires the `GL_OES_texture_npot`,
  `GL_OES_texture_3D` and `GL_OES_depth_texture` extensions for full ps.1.3
  support.

## Requirements

The translator probes several GLES 1.x extensions via `dx8gles11_has_extension()`
and adjusts behaviour when support is missing.

### Mandatory

| Extension             | Notes                                                         |
|-----------------------|---------------------------------------------------------------|
| `GL_OES_multitexture` | Needed for multi‑stage texturing. Compilation fails without it. |

### Optional

| Extension                            | Fallback                                                                |
|--------------------------------------|-------------------------------------------------------------------------|
| `GL_OES_vertex_buffer_object`        | VBO commands are skipped and client arrays are expected.                |
| `GL_OES_texture_matrix`              | Texture matrix uploads are ignored.                                     |
| `GL_OES_texture_npot`                | NPOT uploads set `dx8gles11_error()` and return failure.                |
| `GL_OES_depth_texture`               | Depth textures fail to create with an error.                            |
| `GL_OES_texture_3D`                  | 3D texture uploads fail with an error.                                  |
| `GL_OES_compressed_paletted_texture` | Falls back to uncompressed formats.                                     |

---

## Directory Layout

```
├── CMakeLists.txt          Build script (static lib)
├── include/                Public headers
│   ├── GLES/         		OpenGL ES 1.1 headers
│   ├── dx8gles11.h         Main API + enums
│   ├── preprocess.h        Tiny C pre‑processor
│   ├── dx8asm_parser.h     DX8 ASM → IR structs
│   └── utils.h             Header‑only stretchy buffer
└── src/                    Library sources
    ├── preprocess.c        Pre‑processor impl.
    ├── dx8asm_parser.c     ASM tokeniser / IR builder
    ├── dx8_to_gles11.c     Translator + error text
    └── utils.c             Empty (placeholder for future code)
```

---

## Quick Start

```bash
# 1. Fetch
$ git clone https://github.com/agentdavo/dx8‑to‑gles11.git
$ cd dx8‑to‑gles11

# 2. Build (desktop mock‑GL)
$ mkdir build && cd build
$ cmake ..  # respects $CC / $CFLAGS
$ cmake --build .

# 3. Link
# add build/libdx8gles11.a to your link line, include "dx8gles11.h"
```

### Running Tests

```bash
$ cd build
$ ctest -V
```

The test suite compiles a few sample shaders under `tests/fixtures` and
compares the generated command lists against the files in `tests/expected`.

On Android / iOS set `-DPLATFORM_GLES=ON` or simply ensure the header search path resolves to **Khronos GLES 1.1** headers.

---

## Using the API

```c
#include "dx8gles11.h"

GLES_CommandList cl;
if (dx8gles11_compile_file("shader.asm", NULL, &cl) != 0) {
    fprintf(stderr, "compile failed: %s\n", dx8gles11_error());
    exit(1);
}

/* alternatively: dx8gles11_compile_string(src_text, NULL, &cl); */

for (size_t i = 0; i < cl.count; ++i) {
    const gles_cmd *c = &cl.data[i];
    /* switch (c->type) → issue gl* calls */
}

gles_cmdlist_free(&cl);
```

If the assembly text is already in memory, call `dx8gles11_compile_string()`
instead of `dx8gles11_compile_file()`. The string is preprocessed first so
`#define` and `#include` directives work the same as with file input.

Both compile functions run the same preprocessor. Missing `#include` files or
exceeding shader limits triggers an error via `dx8gles11_error()`.

The sample runtime under `examples/replay_runtime.c` now shows how to bind a VBO
and enable vertex arrays.

## Tools

### Usage coverage script

To gauge how many DirectX opcodes are currently recognised by the translator,
use the helper script under `tools/usage_coverage.py`.  Pass it a text report
listing each opcode and an optional usage count:

```bash
$ python tools/usage_coverage.py my_report.txt
```

Any opcode string found inside `src/` is treated as implemented and a short
coverage summary is printed. The script also writes a Markdown table to
`COVERAGE.md` showing the per-opcode status.

---

## Roadmap

* **v0.2** – constant registers, `def cN, …`, point‑sprite OES support.
* **v0.3** – optional fragment‑shader transpiler for `IMG_texture_env_enhanced_fixed_function`.
* **v1.0** – full DX8 VS coverage, automated test harness with golden JSON.

Community PRs welcome—see `CONTRIBUTING.md` (coming soon).

---

## License

MIT © 2025 Open‑Source Contributors.  Generated Khronos headers retain their original MIT license.
