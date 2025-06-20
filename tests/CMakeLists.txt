add_executable(test_preprocess test_preprocess.c)
target_link_libraries(test_preprocess dx8gles11)
add_test(NAME preprocess_nested_includes COMMAND test_preprocess
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
