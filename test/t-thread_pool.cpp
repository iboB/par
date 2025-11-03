// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <par/thread_pool.hpp>
#include <par/prun.hpp>
#include <doctest/doctest.h>
#include <atomic>
#include <thread>
#include <vector>

TEST_CASE("opts") {
    par::run_opts opts;
    CHECK(opts.sched == par::schedule_dynamic);
    CHECK(opts.max_par == 0);
}

TEST_CASE("external get_par") {
    static constexpr uint32_t num_threads = 4;
    static constexpr uint32_t num_jobs = num_threads + 1;
    par::thread_pool pool("test", num_threads);

    CHECK(pool.get_par() == num_jobs);
    CHECK(pool.get_par({}) == num_jobs);
    CHECK(pool.get_par({.max_par = 1}) == 1);
    CHECK(pool.get_par({.max_par = 3}) == 3);
    CHECK(pool.get_par(5, {.max_par = 3}) == 3);
    CHECK(pool.get_par(2, {.max_par = 3}) == 2);
    CHECK(pool.get_par(-2, {.max_par = 3}) == 0);
    CHECK(pool.get_par({.max_par = num_threads}) == num_threads);
    CHECK(pool.get_par({.max_par = num_jobs}) == num_jobs);
    CHECK(pool.get_par({.max_par = 1000}) == num_jobs);
    CHECK(pool.get_par(2, {.max_par = 1000}) == 2);
    CHECK(pool.get_par(1'000'000'000'000ll, {.max_par = 1000}) == num_jobs);
    CHECK(pool.get_par({.max_par = 0}) == num_jobs);
    CHECK(pool.get_par(2, {.max_par = 0}) == 2);
    CHECK(pool.get_par(2000, {.max_par = 0}) == num_jobs);

    par::run_opts opts;
    opts.max_par = 3;
    CHECK(pool.adjust_par(5, opts) == 3);
    CHECK(opts.max_par == 3);

    CHECK(pool.adjust_par(2, opts) == 2);
    CHECK(opts.max_par == 2);
}

TEST_CASE("thread pool dynamic") {
    static constexpr uint32_t num_threads = 4;
    static constexpr uint32_t num_jobs = num_threads + 1;
    par::thread_pool pool("test", num_threads);

    struct test_task_result {
        uint32_t ret;
        uint32_t calls;
        uint32_t iids;
    };

    auto run_test_task = [&](uint32_t max_par) {
        std::atomic_uint32_t calls = 0;
        std::atomic_uint32_t iids = 0;
        auto ret = prun(pool, {.max_par = max_par}, [&](uint32_t iid) {
            ++calls;
            iids |= (1 << iid);
        });
        return test_task_result{ret, calls.load(), iids.load()};
    };

    SUBCASE("run task in single thread") {
        auto [ret, calls, iids] = run_test_task(1);
        CHECK(ret == 1);
        CHECK(calls == 1);
        CHECK(iids == 1);
    }
    SUBCASE("run task in multiple threads") {
        auto res = run_test_task(3);
        CHECK(res.ret == 3);
        CHECK(res.calls == 3);
        CHECK(res.iids == 0b111);

        res = run_test_task(num_threads);
        CHECK(res.ret == num_threads);
        CHECK(res.calls == num_threads);
        CHECK(res.iids == (1 << num_threads) - 1);

        res = run_test_task(1000);
        CHECK(res.ret == num_jobs);
        CHECK(res.calls == num_jobs);
        CHECK(res.iids == (1 << num_jobs) - 1);

        res = run_test_task(0);
        CHECK(res.ret == num_jobs);
        CHECK(res.calls == num_jobs);
        CHECK(res.iids == (1 << num_jobs) - 1);
    }
}

TEST_CASE("thread pool static") {
    static constexpr uint32_t num_threads = 4;
    static constexpr uint32_t num_jobs = num_threads + 1;
    par::thread_pool pool("test", num_threads);

    std::vector<std::thread::id> thread_ids(num_jobs);


    // collect thread ids
    {
        auto ret = prun(pool, {.sched = par::schedule_static}, [&](uint32_t iid) {
            thread_ids[iid] = std::this_thread::get_id();
        });

        CHECK(thread_ids[0] == std::this_thread::get_id());

        CHECK(ret == num_jobs);
        for (uint32_t i = 0; i < num_jobs; ++i) {
            CHECK(thread_ids[i] != std::thread::id());
            for (uint32_t j = i + 1; j < num_jobs; ++j) {
                CHECK(thread_ids[i] != thread_ids[j]);
            }
        }
    }

    // run on one thread
    {
        auto iid = uint32_t(-1);
        auto tid = std::thread::id();
        auto ret = prun(pool, {.sched = par::schedule_static, .max_par = 1}, [&](uint32_t id) {
            iid = id;
            tid = std::this_thread::get_id();
        });
        CHECK(ret == 1);
        CHECK(iid == 0);
        CHECK(tid == thread_ids[0]);
    }

    auto run_test_task = [&](uint32_t max_par) {
        std::atomic_uint32_t iids = 0;
        auto pret = prun(pool, {.sched = par::schedule_static, .max_par = max_par}, [&](uint32_t iid) {
            iids |= (1 << iid);
            CHECK(std::this_thread::get_id() == thread_ids[iid]);
        });
        auto ret = iids.load();
        CHECK(((1 << pret) - 1) == ret); // all bits from 0 to pret-1 should be set
        return ret;
    };

    CHECK(run_test_task(1) == 1);
    CHECK(run_test_task(3) == 0b111);
    CHECK(run_test_task(num_threads) == (1 << num_threads) - 1);
    CHECK(run_test_task(1000) == (1 << num_jobs) - 1);
    CHECK(run_test_task(0) == (1 << num_jobs) - 1);

    // nesting should throw
    {
        std::atomic_int32_t global = 0;
        std::atomic_int32_t local = 0;
        std::atomic_int32_t throws = 0;
        prun(pool, {}, [&](uint32_t) {
            try {
                prun(pool, {.sched = par::schedule_static, .max_par = 2}, [&](uint32_t) { ++local; });
            }
            catch (const std::runtime_error& err) {
                CHECK(std::string_view(err.what()) == "unsupported nested par call");
                ++throws;
            }
            ++global;
        });
        CHECK(global == num_jobs);
        CHECK(local == 2);
        CHECK(throws == num_threads);
    }
}

TEST_CASE("dynamic nesting") {
    static constexpr uint32_t num_threads = 4;
    static constexpr uint32_t num_jobs = num_threads + 1;
    par::thread_pool pool("test", num_threads);

    auto run_test_task = [&](par::run_opts gopts, par::run_opts lopts) {
        std::atomic_int32_t global = 0;
        std::atomic_int32_t local = 0;
        prun(pool, gopts, [&](uint32_t) {
            auto ret = prun(pool, lopts, [&](uint32_t) { ++local; });
            CHECK(ret == pool.get_par(lopts));
            ++global;
        });
        return std::make_pair(global.load(), local.load());
    };

    SUBCASE("simple") {
        auto [global, local] = run_test_task({}, {.max_par = 2});
        CHECK(global == num_jobs);
        CHECK(local == 2 * num_jobs);
    }
    SUBCASE("in static") {
        auto [global, local] = run_test_task({.sched = par::schedule_static}, {.max_par = 2});
        CHECK(global == num_jobs);
        CHECK(local == 2 * num_jobs);
    }
    SUBCASE("oversub") {
        auto [global, local] = run_test_task({.sched = par::schedule_static, .max_par = 3}, {});
        CHECK(global == 3);
        CHECK(local == 3 * num_threads + 1);
    }
    SUBCASE("dynamic_no_nesting") {
        auto [global, local] = run_test_task(
            {.sched = par::schedule_static},
            {.sched = par::schedule_dynamic_no_nesting, .max_par = 2}
        );
        CHECK(global == num_jobs);
        CHECK(local == num_jobs + 1);
    }
}

TEST_CASE("range task") {
    static constexpr uint32_t num_threads = 4;
    par::thread_pool pool("test", num_threads);

    auto run_test_task = [&](uint32_t max_par) {
        std::atomic_int32_t sum = 0;
        const uint32_t en = std::min(max_par - 1, num_threads) + 1;
        prun(pool, {.max_par = max_par}, [&](par::job_info arg) {
            CHECK(arg.num_jobs == en);
            CHECK(arg.job_index < arg.num_jobs);
            sum += arg.job_index + 1;
        });
        return sum.load();
    };

    CHECK(run_test_task(1) == 1);
    CHECK(run_test_task(3) == 6);
    CHECK(run_test_task(num_threads) == 10);
    CHECK(run_test_task(1000) == 15);
    CHECK(run_test_task(0) == 15);
}

TEST_CASE("global thread pool") {
    std::atomic_int32_t count = 0;
    auto ret = par::prun({}, [&](uint32_t) {
        ++count;
    });
    CHECK(ret >= 1);
    CHECK(count >= 1);
}
