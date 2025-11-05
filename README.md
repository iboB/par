# par

[![Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20) [![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

Simple task parallelism for C++20.

Alternative to the most commonly used subset of OpenMP.

## Example

Also see complete working [examples](example/) in the repo.

### Trivial for loop

#### par

```cpp
const auto data = make_some_data();
par::pfor({/*default run opts*/}, 0, data.size(), [&](int i) {
    compute_something(data[i]);
});
```

#### OpenMP

```cpp
const auto data = make_some_data();
#pragma omp parallel for // implied default options
for (int i = 0; i < data.size(); ++i) {
    compute_something(data[i]);
}
```

### Job-specific data

#### par
```cpp
struct job_data {
    job_data(const par::job_info& info) 
        : rng(info.job_index)
        , dist(0, 1);
    {}
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
};

const auto data = make_some_data();
par::pfor<job_data>({.max_par = 5}, 0, data.size(), [&](int i, job_data& jd) {
    float r = jd.dist(jd.rng);
    monte_carlo_sample(data[i], r);
});
```

#### OpenMP
```cpp
const auto data = make_some_data();
#pragma omp parallel num_threads(5)
{
    std::mt19937 rng(omp_get_thread_num());
    std::uniform_real_distribution<float> dist(0, 1);
    #pragma omp for
    for (int i = 0; i < data.size(); ++i) {
        float r = dist(rng);
        monte_carlo_sample(data[i], r);
    }
}
```

## Features

par notably has less overhead than OpenMP which can be quite significant for small tasks. See more in the [performance document](doc/perf.md).

* `par::thread_pool`: The thread pool. Multiple thread pools can be instantiated. A global one is used by default by runners. The global thread pool is lazily initialized on first use and lives until process termination.
* Runners:
    * `par::prun`: run a generic task in parallel. The provided function receives a job index.
    * `par::pchunk`: run a task in parallel over chunks of work. The provided function receives the chunk range.
    * `par::pfor`: run a for loop in parallel. The provided function receives the current index.
        * allows specifying job-specific data
        * allows specifying chunks of iterations to be processed by each job
* Runner options `par::run_opts`. See [run_opts.hpp](code/par/run_opts.hpp) for details.
    * `.max_par`: maximum parallelism (number of concurrent jobs). Defaults to the number of thread pool threads.
    * `.sched`: scheduling strategy
        * `schedule_dynamic` (default): jobs are assigned dynamically to threads as they finish previous jobs. Suitable for unbalanced workloads.
        * `schedule_static`: each thread is assigned a fixed set of jobs at the start. Suitable for balanced workloads.

### Notable unsupported OpenMP features

* No guided scheduling.
* No barriers or other synchronization primitives.
* Limited nested parallelism support: only dynamically scheduled tasks can be nested.
* No thread ids. Instead `job_index` is used, but with dynamic scheduling multiple job indices may end up being executed by the same thread. Use `std::this_thread::get_id()` if you need the actual thread id.
* No extended features like atomic, SIMD, reductions, etc.

## Usage

Note that par, and code using it, require at least C++20. Any C++20 capable compiler should work.

The easiest, and currently only supported, way to add par to a project is as a [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) package. If you are using this package manager (and you should be), you only need to add this line to your `CMakeLists.txt`: `CPMAddPackage(gh:iboB/par@0.1.0)`. *Update the version "0.1.0" to the one you want.*

In your CMake code link with `par::par`. The relevant CMake configurations are:

* `BUILD_SHARED_LIBS` - respected and controls whether par is built as a shared or static library.
* `par_STATIC` - if set to `ON`, par is always built as a static library, regardless of `BUILD_SHARED_LIBS`

The build is very straightforward and other ways of integration, though not explicitly supported, should work:

* As a submodule/subrepo: par bundles CPM, so this should just work as well, as long as there are no dependency clashes.
* Copy the `code/` subdirectory into your project and make sure the dependencies are available.
* Whatever your heart desires

### Dependencies

The tests, examples, and configuration depend on various packages, but par's code itself only depends on:

* [iboB/splat](https://github.com/iboB/splat)
* [iboB/itlib](https://github.com/iboB/itlib)

These libraries are header-only and have no dependencies of their own. If you copy code or create an alternative build process, things should be relatively easy to set up.

## FAQ

### Why not just use OpenMP?

The initial motivation for this library was that using OpenMP with thread sanitizers leads to a slew of false positive errors. To fix this, one needs to either turn the thread sanitizer off for potentially big chunks of their code, or build OpenMP itself with thread sanitization enabled. Incorporating any of these (especially custom OpenMP) into a build system is very unpleasant.

On the other hand, I only needed a relatively small subset of OpenMP: parallel for loops and basic scheduling.

That's why I set out to reimplement these in a reusable library with the goal of having my code being no slower with it compared to OpenMP.

To my surprise, it not only achieves this, but outperforms OpenMP in some cases, likely because of its simplicity and the lower overhead of managing fewer features.

## Why not use `std::execution::par`?

I don't like `std::execution::par`. It's acceptable for small oneshot tasks, that you run once and forget, but it's terrible for integrating into larger systems with more moving parts. It's a black box. You can't control how many threads it uses, you can't control how tasks are scheduled, you can't integrate it with your own thread pools or task systems. Additionally it only works on iterators which makes certain algorithms harder to express.

## License

This software is distributed under the MIT Software License.

See accompanying file LICENSE or copy [here](https://opensource.org/licenses/MIT).

Copyright &copy; 2025 [Borislav Stanimirov](http://github.com/iboB)
