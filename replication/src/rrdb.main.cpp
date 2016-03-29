// apps
# include "rrdb.app.example.h"
# include "rrdb.server.impl.h"
# include "rrdb.check.h"
# include "pegasus.perf_counter.h"
# include "pegasus_owl_updater.h"
# include "info_collector_app.h"

int main(int argc, char** argv)
{
    // register replication application provider
    dsn::replication::register_replica_provider< ::dsn::apps::rrdb_service_impl>("rrdb");

    // register all possible services
    dsn::register_app< ::dsn::service::meta_service_app>("meta");
    dsn::register_app< ::dsn::replication::replication_service_app>("replica");
    dsn::register_app< ::pegasus::apps::info_collector_app>("collector");
    dsn::register_app< ::dsn::apps::rrdb_client_app>("client");
    dsn::register_app< ::dsn::apps::rrdb_perf_test_client_app>("client.perf");

    // register pegasus profiler
    ::dsn::tools::internal_use_only::register_component_provider(
        "pegasus::tools::pegasus_perf_counter",
        pegasus::tools::pegasus_perf_counter_factory,
        PROVIDER_TYPE_MAIN
        );

    // register global checker if necesary
    dsn_register_app_checker("rrdb.checker",
        ::dsn::apps::rrdb_checker::create< ::dsn::apps::rrdb_checker>,
        ::dsn::apps::rrdb_checker::apply
        );

    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);

    return 0;
}
