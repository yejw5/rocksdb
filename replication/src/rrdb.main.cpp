// apps
# include "rrdb.app.example.h"
# include "rrdb.server.impl.h"
# include "rrdb.check.h"
# include "pegasus.profiler.h"

int main(int argc, char** argv)
{
    // register replication application provider
    dsn::replication::register_replica_provider< ::dsn::apps::rrdb_service_impl>("rrdb");

    // register all possible services
    dsn::register_app< ::dsn::service::meta_service_app>("meta");
    dsn::register_app< ::dsn::replication::replication_service_app>("replica");
    dsn::register_app< ::dsn::apps::rrdb_client_app>("client");
    dsn::register_app< ::dsn::apps::rrdb_perf_test_client_app>("client.perf");

    // register pegasus profiler
    dsn::tools::register_toollet<::dsn::apps::pegasus_profiler>("pegasus_profiler");

    // register global checker if necesary
    dsn_register_app_checker("rrdb.checker",
        ::dsn::apps::rrdb_checker::create< ::dsn::apps::rrdb_checker>,
        ::dsn::apps::rrdb_checker::apply
        );

    // specify what services and tools will run in config file, then run
    dsn_run(argc, argv, true);
    return 0;
}
