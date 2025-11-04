// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "debug_stats.hpp"
#include <ostream>
#include <format>

namespace par {

inline void print_debug_stats(std::ostream& os, const debug_stats& stats) {
    os << "Thread pool \"" << stats.pool_name << "\" debug stats:\n";
    os << std::format("  Total lifetime: {:.3f} ms\n", stats.total_lifetime_ns / 1'000'000.0);
    os << "  Callers:\n";
    os << std::format("    Tasks executed: {}\n", stats.caller_stats.num_tasks_executed.load());
    os << std::format("    Tasks stolen:   {}\n", stats.caller_stats.num_tasks_stolen.load());
    for (size_t i = 0; i < stats.per_worker.size(); ++i) {
        const auto& ws = stats.per_worker[i];
        os << std::format("  Worker {:3}:\n", i);
        os << std::format("    Tasks executed: {}\n", ws->num_tasks_executed.load());
        os << std::format("    Tasks stolen:   {}\n", ws->num_tasks_stolen.load());
        os << std::format("    Total task time: {:.3f} ms\n", ws->total_task_time_ns.load() / 1'000'000.0);
    }
}

} // namespace par