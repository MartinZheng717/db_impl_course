add_test([=[test_bp_manager.test_bp_manager_simple_lru]=]  /home/zhengzhiwei/db_impl_course/build/bin/bp_manager_test [==[--gtest_filter=test_bp_manager.test_bp_manager_simple_lru]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[test_bp_manager.test_bp_manager_simple_lru]=]  PROPERTIES WORKING_DIRECTORY /home/zhengzhiwei/db_impl_course/build/unitest SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  bp_manager_test_TESTS test_bp_manager.test_bp_manager_simple_lru)
