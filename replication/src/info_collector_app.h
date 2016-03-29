#pragma once
#include <dsn/cpp/service_app.h>
#include <dsn/cpp/clientlet.h>
#include "info_collector.h"

namespace pegasus{ namespace apps{

DEFINE_TASK_CODE(LPC_RRDB_COLLECTOR_TIMER, TASK_PRIORITY_COMMON, ::dsn::THREAD_POOL_DEFAULT)

class info_collector_app : public dsn::service_app, public virtual clientlet
{
public:
    info_collector_app();

    ~info_collector_app(void);

    virtual ::dsn::error_code start(int argc, char** argv) override;

    virtual void stop(bool cleanup = false) override;

private:
    void on_collect();

private:
    task_ptr _timer;
    info_collector _collector;
};

}}
