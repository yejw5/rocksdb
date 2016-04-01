#include <dsn/dist/replication.h>
#include <dsn/dist/replication/replication_other_types.h>

#include "info_collector_app.h"

#include <iostream>
#include <fstream>
#include <iomanip>

namespace pegasus{ namespace apps{

info_collector_app::info_collector_app()
{

}

info_collector_app::~info_collector_app()
{
}

dsn::error_code info_collector_app::start(int argc, char** argv)
{
    std::vector< ::dsn::rpc_address> meta_servers;
    ::dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);

    _timer = ::dsn::tasking::enqueue_timer(LPC_RRDB_COLLECTOR_TIMER, this, [this]{on_collect();}, std::chrono::seconds(1));
    return ::dsn::ERR_OK;
}

void info_collector_app::stop(bool cleanup)
{
    _timer->cancel(true);
}

void info_collector_app::on_collect()
{
    _collector.on_collect();
}

}}
