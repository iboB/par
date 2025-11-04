# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
CPMAddPackage(gh:iboB/picobench@2.08)
set(OpenMP_RUNTIME_MSVC llvm)
find_package(OpenMP REQUIRED)

macro(par_benchmark name)
    set(tgt bench-par-${name})
    add_executable(${tgt})
    set_target_properties(${tgt} PROPERTIES FOLDER bench)
    target_sources(${tgt} PRIVATE b-${name}.cpp ${ARGN})
    target_link_libraries(${tgt} PRIVATE
        par::par
        OpenMP::OpenMP_CXX
        picobench::picobench
    )
    add_custom_target(
        run-${tgt}
        COMMAND ${tgt}
    )
    set_target_properties(run-${tgt} PROPERTIES FOLDER bench)
endmacro()

par_benchmark(rejection-sample)
par_benchmark(mandelbrot)
