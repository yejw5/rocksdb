#include "rrdb_client.h"
#include "rrdb_client_factory_impl.h"

namespace dsn{ namespace apps {

bool rrdb_client_factory::initialize(const char* config_file)
{
    return rrdb_client_factory_impl::initialize(config_file);
}

irrdb_client* rrdb_client_factory::get_client(const char* app_name, const char* cluster_meta_servers)
{
    return rrdb_client_factory_impl::get_client(app_name, cluster_meta_servers);
}

}} //namesapce
