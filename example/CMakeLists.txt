# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#

function(par_example name)
    set(tgt example-par-${name})
    add_executable(${tgt})
    set_target_properties(${tgt} PROPERTIES FOLDER example)
    target_sources(${tgt} PRIVATE e-${name}.cpp ${ARGN})
    target_link_libraries(${tgt} PRIVATE
        par::par
    )
endfunction()

par_example(mat-mul)
