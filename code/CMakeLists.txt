# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
include(icm_add_lib)

icm_add_lib(par PAR)
add_library(par::par ALIAS par)

CPMAddPackage(gh:iboB/splat@1.3.4)
CPMAddPackage(gh:iboB/itlib@1.11.9)
target_link_libraries(par PUBLIC
    splat::splat
    itlib::itlib
)

target_sources(par
    INTERFACE FILE_SET HEADERS FILES
        par/api.h

        par/thread_pool.hpp
        par/debug_stats.hpp
        par/debug_stats_print.hpp
    PRIVATE
        par/bits/thread_name.hpp
        par/bits/thread_name.cpp

        par/thread_pool.cpp
)
