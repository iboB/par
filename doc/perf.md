# Performance

Testing the overhead of a parallel execution library is tricky. The overhead is in the microseconds. In real-world scenarios with non-trivial workloads, it is often dwarfed by the actual work being done. Small workloads, where the overhead dominates are subject to noise from the OS scheduler, CPU frequency scaling, background processes, and the like.

## Sleep benchmark

The sleep benchmark runs a workload on 8 threads where each job simply sleeps for a fixed amount of time - 10ms. This would be an example of a perfectly balanced heavy workload.

Here are the results of 20 samples for each of 8, 16, and 32 iterations on Ubuntu 24.04 with gcc 13.3.0 (AMD Ryzen 9 7950X):

`$ ./bin/bench-par-sleep --iters=8,16,32 --samples=20`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par *                    |       8 |    10.265 | 1283080 |      - |      779.4
 openmp                   |       8 |    10.328 | 1290963 |  1.006 |      774.6
 linear                   |       8 |    81.351 |10168920 |  7.925 |       98.3
 par *                    |      16 |    20.437 | 1277311 |      - |      782.9
 openmp                   |      16 |    20.438 | 1277389 |  1.000 |      782.8
 linear                   |      16 |   164.134 |10258370 |  8.031 |       97.5
 par *                    |      32 |    40.638 | 1269945 |      - |      787.4
 openmp                   |      32 |    41.082 | 1283797 |  1.011 |      778.9
 linear                   |      32 |   329.468 |10295890 |  8.107 |       97.1

Similar results are obtained on Windows.

## Mandelbrot benchmark

The Mandelbrot benchmark runs a workload on 8 threads where each job computes a number of pixels of the Mandelbrot set. This benchmark represents a heavily unbalanced workload, as some pixels take much longer to compute than others. A dynamic scheduling strategy is therefore preferred.

The baseline here uses par nesting to express the 2D loop over pixels. The other par version uses a manually collapsed loop. The OpenMP version uses a collapsed pragma. The linear version is single-threaded.

Here are the results of 500 samples for each of 16 and 32 iterations on Ubuntu 24.04 with gcc 13.3.0 (AMD Ryzen 9 7950X CPU):

`$ ./bin/bench-par-mandelbrot --iters=16,32 --samples=500`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par_nest *               |      16 |     0.070 |    4378 |      - |   228369.2
 par_manual_collapse      |      16 |     0.068 |    4247 |  0.970 |   235422.2
 openmp                   |      16 |     0.070 |    4353 |  0.994 |   229677.2
 linear                   |      16 |     0.397 |   24799 |  5.663 |    40323.0
 par_nest *               |      32 |     0.243 |    7602 |      - |   131541.1
 par_manual_collapse      |      32 |     0.241 |    7525 |  0.990 |   132884.8
 openmp                   |      32 |     0.253 |    7900 |  1.039 |   126567.3
 linear                   |      32 |     1.553 |   48522 |  6.383 |    20608.9

Here are the results on Windows 10 with MSVC 19.36.32532.0 (Intel Core i9-13900HX):

`> bin\bench-par-mandelbrot --iters=16,32 --samples=500`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par_nest *               |      16 |     0.056 |    3506 |      - |   285205.0
 par_manual_collapse      |      16 |     0.052 |    3256 |  0.929 |   307101.7
 openmp                   |      16 |     0.062 |    3856 |  1.100 |   259319.3
 linear                   |      16 |     0.347 |   21706 |  6.191 |    46069.7
 par_nest *               |      32 |     0.187 |    5831 |      - |   171489.8
 par_manual_collapse      |      32 |     0.197 |    6153 |  1.055 |   162519.0
 openmp                   |      32 |     0.210 |    6565 |  1.126 |   152308.4
 linear                   |      32 |     1.381 |   43162 |  7.402 |    23168.3


## Rejection sampling benchmark

The rejection sampling benchmark runs a workload on 8 threads where each job performs a number of random samples of points in a cube, and accepts the ones which are in the inscribed sphere: roughly 52% of the samples are accepted. When a sample is accepted, an atomic integer is incremented. This benchmark represents a slightly unbalanced workload, as some jobs will accept more samples than others, and the bottleneck is the atomic increment operation. 

In this case, the difference in OpenMP implementation for different compilers is stark.

First let's take a look at a huge number of small tasks: 1000 tasks, of size 100 and 1000 samples each:

Linux, gcc 13.3.0 (AMD Ryzen 9 7950X):

`$ ./bin/bench-par-rejection-sample --iters=100,1000 --samples=1000`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par *                    |     100 |     0.009 |      89 |      - | 11198208.3
 openmp                   |     100 |     0.002 |      24 |  0.272 | 41152263.4
 linear                   |     100 |     0.002 |      24 |  0.270 | 41493775.9
 par *                    |    1000 |     0.023 |      23 |      - | 42843065.8
 openmp                   |    1000 |     0.009 |       9 |  0.394 |108813928.2
 linear                   |    1000 |     0.023 |      23 |  0.990 | 43269438.8

Note that here OpenMP is much faster than par. About as fast as the linear code. Still, this is for a cost at about 7 microseconds per parallel region.

Now let's look at fewer, larger tasks: 50 tasks, of size 100,000 samples each:

`$ ./bin/bench-par-rejection-sample --iters=100000 --samples=50`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par *                    |  100000 |     0.803 |       8 |      - |124601120.7
 openmp                   |  100000 |     0.888 |       8 |  1.106 |112633541.1
 linear                   |  100000 |     1.550 |      15 |  1.931 | 64524871.1

The difference here is much smaller. Par is slightly faster than OpenMP at about 80 microseconds per parallel region.

This tells us that with this implementation of OpenMP the overhead for an individual iteration is larger than par, but the overhead for launching a parallel region is smaller.

On Windows, msvc 19.36.32532.0 (Intel Core i9-13900HX) the results are quite different:

`> bin\bench-par-rejection-sample --iters=100,1000 --samples=1000`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par *                    |     100 |     0.013 |     126 |      - |  7936507.9
 openmp                   |     100 |     0.028 |     283 |  2.246 |  3533568.9
 linear                   |     100 |     0.002 |      24 |  0.190 | 41666666.7
 par *                    |    1000 |     0.021 |      21 |      - | 47393364.9
 openmp                   |    1000 |     0.037 |      36 |  1.735 | 27322404.4
 linear                   |    1000 |     0.018 |      17 |  0.829 | 57142857.1

Small tasks are much slower with OpenMP here, while par is significantly slower than the linear implementation as it is on Linux.

`> bin\bench-par-rejection-sample --iters=100000 --samples=50`

 Name (* = baseline)      |   Dim   |  Total ms |  ns/op  |Baseline| Ops/second
--------------------------|--------:|----------:|--------:|-------:|----------:
 par *                    |  100000 |     1.342 |      13 |      - | 74515648.3
 openmp                   |  100000 |     1.359 |      13 |  1.012 | 73599764.5
 linear                   |  100000 |     1.533 |      15 |  1.143 | 65214555.9

Here the difference is again gone, with par being slightly faster than OpenMP at about 20 microseconds per parallel region. Notably, though, both par and OpenMP are significantly slower relative to the linear solution than on Linux.
