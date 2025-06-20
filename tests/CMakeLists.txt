add_executable(test_preprocess test_preprocess.c)
target_link_libraries(test_preprocess dx8gles11)
add_test(NAME preprocess_nested_includes COMMAND test_preprocess
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_parse_error test_parse_error.c)
target_link_libraries(test_parse_error dx8gles11)
add_test(NAME parse_error_invalid_instr COMMAND test_parse_error fixtures/invalid_instr.asm
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME parse_error_invalid_const COMMAND test_parse_error fixtures/invalid_const.asm
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
