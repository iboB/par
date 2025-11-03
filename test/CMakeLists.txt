# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
CPMAddPackage(gh:iboB/doctest-util@0.1.4)
set_target_properties(doctest PROPERTIES FOLDER test)
set_target_properties(doctest-main PROPERTIES FOLDER test)

macro(par_test test)
    add_doctest_lib_test(${test} par t-${test}.cpp)
endmacro()

par_test(anchor)
par_test(task_func)

par_test(thread_pool)

par_test(pchunk)
par_test(pfor)
