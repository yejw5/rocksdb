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

    dsn::error_code update_meta();
    //dsn::error_code update_apps();

    // mysql update
    void on_update_meta(dsn::rpc_address addr, dsn::error_code error, configuration_list_apps_response resp);
    //dsn::error_code update_app_info(app_info);

    void end_meta_request(task_ptr callback, int retry_times, error_code err, dsn_message_t request, dsn_message_t resp);

    template<typename TRequest, typename TCallback>
    //where TCallback = void(error_code, TResponse&&)
    //  where TResponse = DefaultConstructible + DSNSerializable
    dsn::task_ptr request_meta(
            dsn::rpc_address target,
            dsn_task_code_t code,
            TRequest&& req,
            TCallback&& callback,
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
            [this, task] (error_code err, dsn_message_t request, dsn_message_t response)
            {
                end_meta_request(std::move(task), 0, err, request, response);
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

    sql::Driver *_driver;
    sql::Connection *_con;
};

}}
