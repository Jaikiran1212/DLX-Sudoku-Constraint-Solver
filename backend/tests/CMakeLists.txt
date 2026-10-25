add_executable(dlx_tests unit_tests.cpp)
target_link_libraries(dlx_tests PRIVATE dlx_core)
add_test(NAME dlx_unit_tests COMMAND dlx_tests)
