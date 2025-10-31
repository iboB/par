# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_executable(par-sandbox
    sandbox.cpp
)
target_link_libraries(par-sandbox par::par)
set_target_properties(par-sandbox PROPERTIES FOLDER sandbox)
