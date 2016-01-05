#include "rrdb_client_factory_impl.h"

namespace dsn{ namespace apps{

std::unordered_map<std::string, rrdb_client_factory_impl::app_to_client_map> rrdb_client_factory_impl::_cluster_to_clients;
dsn::service::zlock* rrdb_client_factory_impl::_map_lock;

bool rrdb_client_factory_impl::initialize(const char* config_file)
{
    dsn_core_init();
    //use config file to run
    char exe[] = "client";
    char config[50];
    sprintf(config, "%s", config_file);
    char* argv[] = { exe, config};
    dsn_run(2, argv, false);
    rrdb_client_impl::init_error();
    _map_lock = new dsn::service::zlock();
    return true;
}

irrdb_client* rrdb_client_factory_impl::get_client(const char* app_name, const char* cluster_name)
{

    std::string section(cluster_name);
    if(!section.empty())
        section += ".replication.meta_servers";
    else
        section += "replication.meta_servers";

    service::zauto_lock l(*_map_lock);
    auto it = _cluster_to_clients.find(section);
    if (it == _cluster_to_clients.end())
    {
        app_to_client_map ac;
        it = _cluster_to_clients.insert(std::unordered_map<std::string, app_to_client_map>::value_type(section, ac)).first;
    }

    app_to_client_map& app_to_clients = it->second;
    auto it2 = app_to_clients.find(std::string(app_name));
    if (it2 == app_to_clients.end())
    {
        std::vector< ::dsn::rpc_address> servers;
        replication_app_client_base::load_meta_servers(servers, section.c_str());
        it2 = app_to_clients.insert(app_to_client_map::value_type(std::string(app_name), new rrdb_client_impl(app_name, cluster_name, servers))).first;
    }
    return it2->second;
}

}}
