# File: AGENTS.md

---

# Guidance for LLM / AI Code Agents

This document gives autonomous or semi‑autonomous coders (GitHub Copilot, ChatGPT Code Interpreter, etc.) the *minimum context* needed to reason about and safely extend the **DX8 → GLES 1.1 Translator** codebase.

## 1. High‑Level Architecture

1. **Pre‑processing** (`src/preprocess.c`)  
   Handles `#include` + simple `#define` (no parameters). Returns a *flat source string*.
2. **Parsing** (`src/dx8asm_parser.c`)  
   Tokenises assembly into `asm_instr` array. Each struct stores opcode + up to 3 operands.
3. **Translation** (`src/dx8_to_gles11.c`)  
   Walks the IR, appending `gles_cmd` records. Each record is an enum + 4×`float` + 4×`uint32` payload.
4. **Runtime Execution** (not yet in tree)  
   The host application drives OpenGL ES by switching on `cmd.type`.

```
DX8 .asm ─┐
          ▼          +────────────+     +───────────────+
preprocess() ──▶ asm_parse() ──▶ translate_instr() ──▶ GLES_CommandList
                                    | array of gles_cmd |
                                    +───────────────+
```

## 2. Adding a New Opcode

1. Update **`dx8gles11.h`** – append a value to `enum gles_cmd_type` *only if* a brand‑new fixed‑function behaviour is needed. Prefer re‑using existing commands.
2. Extend **`translate_instr()`** in `src/dx8_to_gles11.c`:
   ```c
   if (!strcmp(inst->opcode, "dp3")) {
       gles_cmd c = {.type = GLES_CMD_TEX_ENV_COMBINE};
       c.u[0] = GL_COMBINE;
       c.u[1] = GL_DOT3_RGB;
       cmdlist_push(out, c);
       return;
   }
   ```
3. Keep the function *simple*—a handful of `if` branches. When this grows past ~30 cases consider a look‑up table.

## 3. Coding Standards

* **C11**, portable CMake, zero external deps (no STL, no POSIX beyond `stdio.h`).
* Stretchy‑buffer macros live in `include/utils.h`. They must stay header‑only.
* No dynamic linking—static `.a` only, to fit embedded deliveries.
* Preserve `clang‑format` defaults: 4‑space indent, 100‑col width.

## 4. Error Handling

* Functions return **0 on success**, \<0 on failure.
* `dx8gles11_error()` *always* holds a last human‑readable description—do **not** attempt to send heap pointers to callers.

## 5. Future Automation Ideas

* **Test Generation** – feed each `.asm` in `tests/fixtures` through the compiler, dump JSON, compare against references.
* **Optimisation Pass** – AI may explore peephole rules, e.g. `mov r0, r0` → drop.
* **Doc Bot** – auto‑update `README.md` opcode table when `translate_instr()` gains cases.

## 6. Known Limitations / Non‑Goals

* No fragment shader transpilation (out of scope until ES 2.0 path lands).
* No macro parameters in the pre‑processor.
* No flow‑control (DX8 `if`/`rep`)—would need runtime code injection which GLES 1.x cannot emulate.

## 7. Safety Checks for Agents

* **Do not** introduce allocation in inner loops; use stretchy‑buffer where possible.
* **Do not** include C++ code—the compilation is `LANGUAGES C`.
* **Do not** add *new* external dependencies (zlib, stb, etc.) without a human‑review gate.
* **Run `ctest -V`** before proposing a PR (tests will be added in v0.2).

---

Happy hacking! The maintainers welcome AI‑generated contributions as long as they adhere to the constraints above and compile on *both* desktop GL stubs and real GLES 1.1 hardware.

