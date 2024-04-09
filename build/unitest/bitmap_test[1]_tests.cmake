add_test([=[test_bitmap.test_bitmap]=]  /home/zhengzhiwei/db_impl_course/build/bin/bitmap_test [==[--gtest_filter=test_bitmap.test_bitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[test_bitmap.test_bitmap]=]  PROPERTIES WORKING_DIRECTORY /home/zhengzhiwei/db_impl_course/build/unitest SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  bitmap_test_TESTS test_bitmap.test_bitmap)
