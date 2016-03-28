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

irrdb_client* rrdb_client_factory_impl::get_client(const char* app_name, const char* cluster_meta_servers)
{
    if (app_name == nullptr || cluster_meta_servers == nullptr) {
        derror("invalid parameter");
        return nullptr;
    }

    std::vector< ::dsn::rpc_address> servers;
    if (cluster_meta_servers[0] != '\0') {
        std::vector<std::string> server_strs;
        ::dsn::utils::split_args(cluster_meta_servers, server_strs, ',');
        if (server_strs.empty()) {
            derror("invalid parameter");
            return nullptr;
        }
        for (auto& addr_str : server_strs) {
            ::dsn::rpc_address addr;
            if (!addr.from_string_ipv4(addr_str.c_str())) {
                derror("invalid parameter");
                return nullptr;
            }
            servers.push_back(addr);
        }
    }

    service::zauto_lock l(*_map_lock);
    auto it = _cluster_to_clients.find(cluster_meta_servers);
    if (it == _cluster_to_clients.end())
    {
        app_to_client_map ac;
        it = _cluster_to_clients.insert(std::unordered_map<std::string, app_to_client_map>::value_type(cluster_meta_servers, ac)).first;
    }

    app_to_client_map& app_to_clients = it->second;
    auto it2 = app_to_clients.find(app_name);
    if (it2 == app_to_clients.end())
    {
        if (cluster_meta_servers[0] == '\0') {
            replication_app_client_base::load_meta_servers(servers, "replication.meta_servers");
        }
        rrdb_client_impl* client = new rrdb_client_impl(app_name, servers);
        it2 = app_to_clients.insert(app_to_client_map::value_type(app_name, client)).first;
        std::string real_servers = client->get_cluster_meta_servers();
        if (real_servers != cluster_meta_servers)
        _cluster_to_clients[real_servers][app_name] = client;
    }
    return it2->second;
}

}}
