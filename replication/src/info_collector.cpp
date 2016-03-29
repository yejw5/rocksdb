#include <iostream>
#include "info_collector.h"

using namespace dsn;
using namespace dsn::replication;

namespace pegasus{ namespace apps{

info_collector::info_collector()
{
    replication_app_client_base::load_meta_servers(meta_server_vector);

    _meta_servers.assign_group(dsn_group_build("meta.servers"));
    for (auto& m : meta_server_vector)
        dsn_group_add(_meta_servers.group_handle(), m.c_addr());

    _mysql_host = dsn_config_get_value_string("info_collector", "host", "", "mysql host");
    _mysql_user = dsn_config_get_value_string("info_collector", "user", "", "mysql user");
    _mysql_passwd = dsn_config_get_value_string("info_collector", "passwd", "", "mysql password");
    _mysql_database = dsn_config_get_value_string("info_collector", "database", "", "mysql database");

    // initialize mysql server connection
    _driver = get_driver_instance();

    _con = _driver->connect(_mysql_host, _mysql_user, _mysql_passwd);
    _con->setSchema(_mysql_database);
}

void info_collector::on_collect()
{
    if(_flag.test_and_set())
        return;
    update_meta();
    //update_apps();
}

dsn::error_code info_collector::update_meta()
{
    std::shared_ptr<configuration_list_apps_request> req(new configuration_list_apps_request());
    req->status = AS_INVALID;
    for(auto& m : meta_server_vector)
    {
        dsn_group_set_leader(_meta_servers.group_handle(), m.c_addr());
        auto resp_task = request_meta(
            RPC_CM_LIST_APPS,
            req
            );
        resp_task->wait();

        if (resp_task->error() == dsn::ERR_OK)
        {
            update_meta_info(m);
        }
    }
}


dsn::error_code info_collector::update_meta_info(dsn::rpc_address addr)
{
    sql::Statement* stmt = _con->createStatement();
    sql::ResultSet* res = stmt->executeQuery("SELECT 'host' AS _message from monitor_task");

    while (res->next()) {
      std::cout << "\t... MySQL replies: ";
      /* Access column data by alias or column name */
      std::cout << res->getString("_message") << std::endl;
      std::cout << "\t... MySQL says it again: ";
      /* Access column fata by numeric offset, 1 is the first column */
      std::cout << res->getString(1) << std::endl;
    }
    return ERR_OK;
}

dsn::error_code info_collector::update_apps()
{
    std::shared_ptr<configuration_list_apps_request> req(new configuration_list_apps_request());
    req->status = AS_INVALID;
    auto resp_task = request_meta(
        RPC_CM_LIST_APPS,
        req
        );

    resp_task->wait();

    if (resp_task->error() != dsn::ERR_OK)
    {
        return resp_task->error();
    }

    dsn::replication::configuration_list_apps_response resp;
    ::unmarshall(resp_task->response(), resp);
    if(resp.err != dsn::ERR_OK)
    {
        return resp.err;
    }

    for(int i = 0; i < resp.infos.size(); i++)
    {
        app_info info = resp.infos[i];
        update_app_info(info);
    }
}

// mysql updates
dsn::error_code info_collector::update_app_info(app_info info)
{

}


// request
void info_collector::end_meta_request(task_ptr callback, int retry_times, error_code err, dsn_message_t request, dsn_message_t resp)
{
    if(err == dsn::ERR_TIMEOUT && retry_times < 5)
    {
        rpc_address leader = dsn_group_get_leader(_meta_servers.group_handle());
        rpc_address next = dsn_group_next(_meta_servers.group_handle(), leader.c_addr());
        dsn_group_set_leader(_meta_servers.group_handle(), next.c_addr());

        rpc::call(
            _meta_servers,
            request,
            this,
            [=, callback_capture = std::move(callback)](error_code err, dsn_message_t request, dsn_message_t response)
            {
                end_meta_request(std::move(callback_capture), retry_times + 1, err, request, response);
            },
            0
         );
    }
    else
        callback->enqueue_rpc_response(err, resp);
}

}} // namespace
