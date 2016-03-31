#include <iostream>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include "info_collector.h"

using namespace dsn;
using namespace dsn::replication;

namespace pegasus{ namespace apps{

info_collector::info_collector()
{
    replication_app_client_base::load_meta_servers(meta_server_vector);

    _meta_servers.assign_group(dsn_group_build("meta.servers"));
    for (auto& m : meta_server_vector)
    {
        dsn_group_add(_meta_servers.group_handle(), m.c_addr());
    }

    for (int i = 0; i < meta_server_vector.size(); i++)
    {
        dsn::rpc_address target;
        target.assign_group(dsn_group_build("meta_leader"));
        for (auto& n : meta_server_vector)
        {
            dsn_group_add(target.group_handle(), n.c_addr());
        }
        _meta_leaders.push_back(target);
    }

    _mysql_host = dsn_config_get_value_string("info_collector", "host", "", "mysql host");
    _mysql_user = dsn_config_get_value_string("info_collector", "user", "", "mysql user");
    _mysql_passwd = dsn_config_get_value_string("info_collector", "passwd", "", "mysql password");
    _mysql_database = dsn_config_get_value_string("info_collector", "database", "", "mysql database");

    // initialize mysql server connection
    _driver = get_driver_instance();
    _flag.clear();
}

void info_collector::on_collect()
{
    if(_flag.test_and_set())
        return;
    _con = _driver->connect(_mysql_host, _mysql_user, _mysql_passwd);
    _con->setSchema(_mysql_database);

    _running_count.fetch_add(1);
    update_meta();
    //update_apps();
    on_finish();
}

dsn::error_code info_collector::update_meta()
{
    _running_count.fetch_add(meta_server_vector.size());
    for(int i = 0; i < meta_server_vector.size(); i++)
    {
        configuration_list_apps_request req;
        req.status = AS_INVALID;

        auto m = meta_server_vector[i];
        dsn::rpc_address target = _meta_leaders[i];
        dsn_group_set_leader(target.group_handle(), m.c_addr());

        //typedef void* callback(dsn::error_code, configuration_list_apps_response);
        //callback* cb = std::bind(&info_collector::on_update_meta, this, m, std::placeholders::_1, std::placeholders::_2).target<callback>();
        //request_meta(target, RPC_CM_LIST_APPS, req, cb);
        request_meta(target, RPC_CM_LIST_APPS, req, std::forward<std::function<void(dsn::error_code, configuration_list_apps_response)>>(std::bind(&info_collector::on_update_meta, this, m, std::placeholders::_1, std::placeholders::_2)));
    }
}


void info_collector::on_update_meta(dsn::rpc_address addr, dsn::error_code error, configuration_list_apps_response resp)
{
    if(error != dsn::ERR_OK)
    {
        derror("update meta fail, error = %s", dsn_error_to_string(error));
        on_finish();
        return;
    }
    else
    {
        struct sockaddr_in addr2;
        if (inet_pton(AF_INET, "10.108.187.16", &addr2.sin_addr) < 0)
        {
            derror("pton error\n");
            on_finish();
            return;
        }
        char hn[100];
        addr2.sin_family = AF_INET;
        if (getnameinfo((struct sockaddr*)&addr2, sizeof(sockaddr), hn, sizeof(hn), nullptr, 0, NI_NAMEREQD))
        {
            derror("could not resolve hostname\n");
            on_finish();
            return;
        }
        sql::Statement* stmt = _con->createStatement();
        std::stringstream ss;
        ss << "UPDATE monitor_task SET last_attempt_time=now(), last_success_time=now() WHERE host='" << hn << "' AND port=" << addr.port() << ";";
        try {
            int count = stmt->executeUpdate(ss.str().c_str());
            std::cout << "update count:" << count << std::endl;
        } catch (sql::SQLException &e) {
            derror("SQLException, code:%d, err:%s, state:%s", e.getErrorCode(), e.what(), e.getSQLState().c_str());
            on_finish();
        }
    }
}

/*
dsn::error_code info_collector::update_apps()
{
    std::shared_ptr<configuration_list_apps_request> req(new configuration_list_apps_request());
    req->status = AS_INVALID;
    auto resp_task = request_meta(
        RPC_CM_LIST_APPS,
        req,

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
}*/

// on finish
void info_collector::on_finish()
{
    int count = _running_count.fetch_sub(1);
    if(count == 0)
    {
        _flag.clear();
    }
}

// mysql updates
/*
dsn::error_code info_collector::update_app_info(app_info info)
{

}
*/

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
