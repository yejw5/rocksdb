#pragma once
#include <dsn/cpp/clientlet.h>
#include <dsn/dist/replication.h>
#include <dsn/dist/replication/replication_other_types.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace dsn;
using namespace dsn::replication;

namespace pegasus{ namespace apps{

class info_collector : public virtual clientlet
{
public:
    info_collector();

    void on_collect();

private:
    void on_finish();

    void update_meta();
    //dsn::error_code update_apps();

    // mysql update
    void on_update_meta(dsn::rpc_address all, dsn::rpc_address addr, dsn::error_code error, configuration_list_apps_response resp);
    //dsn::error_code update_app_info(app_info);

    sql::ResultSet* selectApp(std::stringstream &ss);
    void updateApp(std::stringstream &ss);
    void exeTransaction(std::stringstream &ss);
    void getHostName(char hn[], const dsn::rpc_address& addr, int len = 100);
    void delUndesiredInPartitionByAppId(int32_t app_id);
    bool updatePegasusToTask(configuration_query_by_index_response &resp, const dsn::replication::partition_configuration &p, int cluster_id);

    void update_apps();
    void on_update_apps(dsn::error_code error, configuration_list_apps_response resp);

    void on_update_partitions(dsn::rpc_address addr, dsn::error_code error, configuration_query_by_index_response resp);

    void end_meta_request(task_ptr callback, int retry_times, error_code err, dsn_message_t request, dsn_message_t resp, bool is_retry);

    template<typename TRequest, typename TCallback>
    //where TCallback = void(error_code, TResponse&&)
    //  where TResponse = DefaultConstructible + DSNSerializable
    dsn::task_ptr request_meta(
            dsn::rpc_address target,
            dsn_task_code_t code,
            TRequest&& req,
            TCallback&& callback,
            bool is_retry = true,
            int timeout_milliseconds= 0,
            int reply_hash = 0
            )
    {
        dsn_message_t msg = dsn_msg_create_request(code, timeout_milliseconds, 0);
        task_ptr task = ::dsn::rpc::create_rpc_response_task(msg, nullptr, std::forward<TCallback>(callback), reply_hash);
        ::marshall(msg, std::forward<TRequest>(req));
        rpc::call(
            target,
            msg,
            this,
            [this, task, is_retry] (error_code err, dsn_message_t request, dsn_message_t response)
            {
                end_meta_request(std::move(task), 0, err, request, response, is_retry);
            },
            0
         );
        return task;
    }

private:
    std::vector<dsn::rpc_address> meta_server_vector;
    std::vector<dsn::rpc_address> _meta_leaders;
    dsn::rpc_address _meta_servers;

    std::atomic_flag _flag;
    std::atomic<int> _running_count;

    std::string _mysql_host;
    std::string _mysql_user;
    std::string _mysql_passwd;
    std::string _mysql_database;

    std::string _cluster_name;

    dsn::service::zlock _lock;
    sql::Driver *_driver;
    sql::Connection *_con;
};

}}
