#pragma once
#include "api.h"
#include <cstdint>

// utility functions for the global thread pool which can be invoked
// without including the full thread_pool.hpp header

namespace par {

class thread_pool;

PAR_API uint32_t suggest_global_thread_pool_size();

PAR_API thread_pool& global_thread_pool();
PAR_API thread_pool& init_global_thread_pool(uint32_t nthreads);
PAR_API thread_pool& init_and_warmup_global_thread_pool(uint32_t nthreads = suggest_global_thread_pool_size());

} // namespace par
