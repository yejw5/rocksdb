#include <iostream>
#include "info_collector.h"
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
 #include <iomanip>
#include  <sys/types.h>
#include  <sys/socket.h>

using namespace dsn;
using namespace dsn::replication;

namespace pegasus{ namespace apps{

info_collector::info_collector()
{
    std::cout << "ctor" << std::endl;
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
    _flag.clear();
}

void info_collector::on_collect()
{
    std::cout << "on collect" << std::endl;
    if(_flag.test_and_set())
        return;
    update_meta();
    //update_apps();
    _flag.clear();
}

dsn::error_code info_collector::update_meta()
{
    std::cout << "update meta" << std::endl;

    std::shared_ptr<configuration_list_apps_request> req(new configuration_list_apps_request());
    req->status = AS_INVALID;
    for(auto& m : meta_server_vector)
    {
        dsn_group_set_leader(_meta_servers.group_handle(), m.c_addr());
        auto resp_task = request_meta(
            RPC_CM_LIST_APPS,
            req
            );
        std::cout << "before wait" << std::endl;
        resp_task->wait();
        std::cout << "after wait" << std::endl;


        if (resp_task->error() == dsn::ERR_OK)
        {
            std::cout << "ok" << std::endl;
            update_meta_info(m);
        }
    }
}


dsn::error_code info_collector::update_meta_info(dsn::rpc_address addr)
{
    //struct sockaddr _addr;
    struct sockaddr_in addr2;// = (struct sockaddr_in*)&_addr;
    //addr2->sin_port = htonl(addr.port());
    //addr2->sin_addr.s_addr = htonl(addr.ip());
    if (inet_pton(AF_INET, "10.108.187.16", &addr2.sin_addr) < 0)
           printf("pton error\n");
    std::cout << addr.ip() << ":" << addr.port() << std::endl;
    char hn[100];
    char sn[100];
    addr2.sin_port = htons(80);
    addr2.sin_family = AF_INET;
    
    if (getnameinfo((struct sockaddr*)&addr2, sizeof(sockaddr), hn, sizeof(hn), sn, sizeof(sn), NI_NAMEREQD))
        printf("could not resolve hostname\n");
    else
        printf("host=%s\n", hn);
    sql::Statement* stmt = _con->createStatement();
    std::stringstream ss;
    ss << "UPDATE monitor_task SET last_attempt_time=now(), last_success_time=now() WHERE host='" << hn << "' AND port=" << addr.port() << ";";
    std::cout << ss.str() << std::endl;
    //sql::ResultSet* res = stmt->executeQuery("SELECT host AS _message from monitor_task");
    
    try {
        int count = stmt->executeUpdate(ss.str().c_str());
        std::cout << "update count:" << count << std::endl;
} catch (sql::SQLException &e) {
  std::cout << "# ERR: SQLException in " << __FILE__;
  std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
  std::cout << "# ERR: " << e.what();
  std::cout << " (MySQL error code: " << e.getErrorCode();
  std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
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
