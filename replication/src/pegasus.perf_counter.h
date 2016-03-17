#pragma once

#include <dsn/tool_api.h>
#include <shm_counter.h>

using namespace dsn;

namespace pegasus{ namespace tools{
    extern __thread falcon::ShmCounter tls_counter;

    perf_counter* pegasus_perf_counter_factory(
        const char* app,
        const char *section,
        const char *name,
        dsn_perf_counter_type_t type,
        const char *dsptr
        );
}} // namespace
