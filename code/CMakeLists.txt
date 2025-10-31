# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
include(icm_add_lib)

icm_add_lib(par PAR)
add_library(par::par ALIAS par)

CPMAddPackage(gh:iboB/splat@1.3.4)
target_link_libraries(par PUBLIC
    splat::splat
)

target_sources(par
    INTERFACE FILE_SET HEADERS FILES
        par/api.h
    PRIVATE
)

