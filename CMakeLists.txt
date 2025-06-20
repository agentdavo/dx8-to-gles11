# =============================================================
#  DirectX 8 Shader → OpenGL ES 1.1 Fixed‑Function Translator
#  ---------------------------------------------------------
#  Project layout
#
#  ├── CMakeLists.txt
#  ├── include/
#  │   ├── dx8gles11.h          (public API + enums)
#  │   ├── preprocess.h         (tiny C pre‑processor)
#  │   ├── dx8asm_parser.h      (DX8 ASM → IR)
#  │   └── utils.h              (stretchy‑buffer helpers)
#  └── src/
#      ├── preprocess.c
#      ├── dx8asm_parser.c
#      ├── dx8_to_gles11.c      (translator + error handling)
#      └── utils.c
# =============================================================

# -------------------------------------------------------------
#  File: CMakeLists.txt
# -------------------------------------------------------------


cmake_minimum_required(VERSION 3.16)
project(DX8toGLES11 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_EXAMPLES "Build replay_runtime example" ON)
option(BUILD_BENCHMARKS "Build benchmark tools" ON)
enable_testing()

add_library(dx8gles11 STATIC
    src/preprocess.c
    src/dx8asm_parser.c
    src/dx8_to_gles11.c
    src/utils.c
    src/lf_queue.c
    src/runtime_pipeline.c
)

target_include_directories(dx8gles11 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(
    TARGETS dx8gles11
    EXPORT dx8gles11Targets
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
install(
    EXPORT dx8gles11Targets
    FILE dx8gles11Config.cmake
    NAMESPACE dx8::
    DESTINATION lib/cmake/dx8gles11
)

if(BUILD_EXAMPLES)
    find_package(Threads REQUIRED)
    add_executable(replay_runtime
        examples/replay_runtime.c
        src/minithread.c)
    target_link_libraries(replay_runtime dx8gles11 Threads::Threads)
    if(NOT EMSCRIPTEN)
        find_package(OpenGL REQUIRED)
        target_link_libraries(replay_runtime OpenGL::GL)
    endif()
endif()

if(BUILD_BENCHMARKS)
    find_package(Threads REQUIRED)
    add_executable(bench_translate
        tools/bench_translate.c
        src/minithread.c)
    target_link_libraries(bench_translate dx8gles11 Threads::Threads)
    add_executable(bench_tests
        tools/bench_tests.c
        src/minithread.c)
    target_link_libraries(bench_tests dx8gles11 Threads::Threads)
    if(NOT EMSCRIPTEN)
        find_package(OpenGL REQUIRED)
        target_link_libraries(bench_translate OpenGL::GL)
        target_link_libraries(bench_tests OpenGL::GL)
    endif()
endif()

enable_testing()
add_subdirectory(tests)
