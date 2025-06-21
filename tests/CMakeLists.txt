add_executable(test_preprocess test_preprocess.c)
target_link_libraries(test_preprocess dx8gles11)
add_test(NAME preprocess_nested_includes COMMAND test_preprocess
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_missing_include test_missing_include.c)
target_link_libraries(test_missing_include dx8gles11)
add_test(NAME preprocess_missing_include COMMAND test_missing_include
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_parse_error test_parse_error.c)
target_link_libraries(test_parse_error dx8gles11)
add_test(NAME parse_error_invalid_instr COMMAND test_parse_error fixtures/invalid_instr.asm
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME parse_error_invalid_const COMMAND test_parse_error fixtures/invalid_const.asm
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(test_compile test_compile.c)
target_link_libraries(test_compile dx8gles11)
foreach(f mov_tex mul_const dp3_matrix add matrix_ops tex_ops terrain_ps motion_blur_vs
             river_water_ps water_reflection_ps water_trapezoid_ps max_min cnd nop)
    add_test(NAME compile_${f}
        COMMAND test_compile ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${f}.asm
                              ${CMAKE_CURRENT_SOURCE_DIR}/expected/${f}.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
