#include <iostream>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include "info_collector.h"
#include <iomanip>
#include <vector>

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

    _mysql_host = dsn_config_get_value_string("pegasus.collector", "host", "", "mysql host");
    _mysql_user = dsn_config_get_value_string("pegasus.collector", "user", "", "mysql user");
    _mysql_passwd = dsn_config_get_value_string("pegasus.collector", "passwd", "", "mysql password");
    _mysql_database = dsn_config_get_value_string("pegasus.collector", "database", "", "mysql database");
    _cluster_name = dsn_config_get_value_string("pegasus.collector", "cluster", "", "cluster name");

    // initialize mysql server connection
    _driver = get_driver_instance();

    _flag.clear();
}

void info_collector::on_collect()
{
    ////std::cout << "before test: " << _running_count << ' ' << std::endl;
    if(_flag.test_and_set())
    {
        //std::cout << "in on_collect(): " << _running_count << std::endl;
        return;
    }

    try {
        sql::ConnectOptionsMap connection_properties;
        connection_properties["OPT_RECONNECT"]=true;
        connection_properties["hostName"] = _mysql_host;
        connection_properties["userName"] = _mysql_user;
        connection_properties["password"] = _mysql_passwd;
        connection_properties["OPT_CONNECT_TIMEOUT"] = 20;

        //TODO: COMMENT
        connection_properties["CLIENT_MULTI_STATEMENTS"]=(true);

        _con = _driver->connect(connection_properties);
        _con->setSchema(_mysql_database);

        //_con -> setAutoCommit(0);
    }
    catch (sql::SQLException &e) {
        derror("SQLException, code:%d, err:%s, state:%s", e.getErrorCode(), e.what(), e.getSQLState().c_str());
        _flag.clear();
        return;
    }

    _running_count.fetch_add(1);
    //std::cout << "before update_meta: " << _running_count << ' ' << std::endl;
    update_meta();
    //std::cout << "before update_apps: " << _running_count << ' ' << std::endl;
    update_apps();
    on_finish();
}

sql::ResultSet* info_collector::selectApp(std::stringstream &ss)
{
    try {
        std::cout << ss.str() <<  std::endl;
        sql::Statement* stmt = _con->createStatement();
        sql::ResultSet *res = stmt->executeQuery(ss.str().c_str());

        delete stmt;
        return res;
    } catch (sql::SQLException &e) {
        derror("SQLException, code:%d, err:%s, state:%s", e.getErrorCode(), e.what(), e.getSQLState().c_str());
    }
}

void info_collector::updateApp(std::stringstream &ss)
{
    try {
        std::cout << ss.str() << std::endl;
        sql::Statement* stmt = _con->createStatement();
        int count = stmt->executeUpdate(ss.str().c_str());
        std::cout << "update count:" << count << std::endl;
        delete stmt;
    } catch (sql::SQLException &e) {
        derror("SQLException, code:%d, err:%s, state:%s", e.getErrorCode(), e.what(), e.getSQLState().c_str());
    }
}

void info_collector::exeTransaction(std::stringstream &ss)
{
    try {
        std::cout << "in transaction: "<< ss.str() << std::endl;
        sql::Statement* stmt = _con->createStatement();
        stmt -> execute ("START TRANSACTION;");
        stmt->executeUpdate(ss.str().c_str());
        stmt -> execute ("COMMIT;");
        //std::cout << "update count:" << count << std::endl;
        delete stmt;
    } catch (sql::SQLException &e) {
        derror("SQLException, code:%d, err:%s, state:%s", e.getErrorCode(), e.what(), e.getSQLState().c_str());
    }
}

void info_collector::update_meta()
{
    _running_count.fetch_add(meta_server_vector.size());
    //std::cout << "in update_meta: " << _running_count << ' ' << std::endl;
    for(int i = 0; i < meta_server_vector.size(); i++)
    {
        configuration_list_apps_request req;
        req.status = AS_INVALID;

        auto m = meta_server_vector[i];
        dsn::rpc_address target = _meta_leaders[i];
        dsn_group_set_leader(target.group_handle(), m.c_addr());
        request_meta(target, RPC_CM_LIST_APPS, req, std::forward<std::function<void(dsn::error_code, configuration_list_apps_response)>>
                     (std::bind(&info_collector::on_update_meta, this, target, m, std::placeholders::_1, std::placeholders::_2)), false);
    }
}

void info_collector::on_update_meta(dsn::rpc_address all, dsn::rpc_address addr, dsn::error_code error, configuration_list_apps_response resp)
{
    if(error != dsn::ERR_OK)
    {
        derror("update meta fail, error = %s", dsn_error_to_string(error));
        char hn[100];
        getHostName(hn, addr);
        dsn::service::zauto_lock l(_lock);
        std::stringstream ss;
        ss << "UPDATE monitor_task SET last_attempt_time=now(), master=";
        ss << 0 << " WHERE host='" << hn << "' AND port=" << addr.port() << ";";
        updateApp(ss);
        on_finish();
        return;
    }
    else
    {
        char hn[100];
        getHostName(hn, addr);

        dsn::service::zauto_lock l(_lock);
        std::stringstream ss;
        ss << "UPDATE monitor_task SET last_attempt_time=now(), last_success_time=now() , master=";
        ss << dsn_group_is_leader(all.group_handle(), addr.c_addr())?"1":"0";
        ss << " WHERE host='";
        ss << hn << "' AND port=" << addr.port() << ";";
        updateApp(ss);
        on_finish();
    }
}

void info_collector::update_apps()
{
    _running_count.fetch_add(1);
    //std::cout << "in update_apps: " << _running_count << ' ' << std::endl;
    configuration_list_apps_request req;
    req.status = AS_INVALID;
    request_meta(_meta_servers, RPC_CM_LIST_APPS, req,
                 std::forward<std::function<void(dsn::error_code, configuration_list_apps_response)>>
                 (std::bind(&info_collector::on_update_apps, this, std::placeholders::_1, std::placeholders::_2)));
}

void info_collector::delUndesiredInPartitionByAppId(int32_t app_id)
{
    std::stringstream ss;
    ss << "DELETE FROM monitor_pegasus_partition_to_task WHERE partition_id IN (SELECT id FROM monitor_pegasus_partition WHERE app_id=" << app_id << ");";
    ss << "DELETE FROM monitor_pegasus_partition WHERE app_id=" << app_id << ";";
    exeTransaction(ss);
}

void info_collector::on_update_apps(dsn::error_code error, configuration_list_apps_response resp)
{
    if(error != dsn::ERR_OK)
    {
        derror("update apps fail, error = %s", dsn_error_to_string(error));
        on_finish();
        return;
    }
    else
    {
        int cluster_id;
        sql::ResultSet *res;
        dsn::service::zauto_lock l(_lock);
        std::stringstream ss;
        ss << "SELECT id from monitor_cluster where name = '";
        ss << _cluster_name << "';";
        res = selectApp(ss);
        if ( !res->next() )
        {
            derror("No next value in \"SELECT id from monitor_cluster ***\"");
            delete res;
            on_finish();
            return;
        }
        else
            cluster_id = res->getInt(1);
        delete res;

        std::stringstream ss_app;
        for(int i = 0; i < resp.infos.size(); i++)
        {
            std::stringstream ss1;
            dsn::replication::app_info info = resp.infos[i];

            ss1 << "SELECT count(*) from monitor_pegasus_app where app_id = '" << info.app_id << "' AND cluster_id = '" << cluster_id << "';";
            res = selectApp(ss1);
            bool isExist;
            if ( !res->next() )
            {
                derror("No next value");
                on_finish();
                delete res;
                return;
            }
            else
                isExist = res->getInt(1) > 0 ? true : false;
            delete res;

            if ( !isExist )
                ss_app << "INSERT into monitor_pegasus_app(app_id, name, type, status, partition_count, cluster_id) "
                   << "values('" << info.app_id << "','" << info.app_name << "','" << info.app_type << "','"
                   << enum_to_string(info.status) << "','" << info.partition_count << "','" << cluster_id << "');";
            else
            {
                ss_app << "UPDATE monitor_pegasus_app SET name = '" << info.app_name << "', type = '" << info.app_type
                   << "', status = '" << enum_to_string(info.status) << "', partition_count = '" << info.partition_count
                   << "' WHERE app_id = '" << info.app_id << "' AND cluster_id = '" << cluster_id << "';";
            }
        }
        exeTransaction(ss_app);

        for(int i = 0; i < resp.infos.size(); i++)
        {
            dsn::replication::app_info info = resp.infos[i];
            if ( info.status == AS_DROPPED )
            {
                //TODO
                //avoid delUndesiredInPartitionByAppId periodly, even it has been execute before
                delUndesiredInPartitionByAppId(info.app_id);
                continue;
            }

            _running_count.fetch_add(1);
            configuration_query_by_index_request req;
            req.app_name = info.app_name;
            request_meta(_meta_servers, RPC_CM_QUERY_PARTITION_CONFIG_BY_INDEX, req,
                         std::forward<std::function<void(dsn::error_code, dsn::replication::configuration_query_by_index_response)>>
                         (std::bind(&info_collector::on_update_partitions, this, _meta_servers, std::placeholders::_1, std::placeholders::_2)));
        }
        on_finish();
    }
}

void info_collector::on_update_partitions(dsn::rpc_address addr, dsn::error_code error, configuration_query_by_index_response resp)
{
    if(error != dsn::ERR_OK)
    {
        derror("update partitions fail, error = %s", dsn_error_to_string(error));
        on_finish();
        return;
    }
    else
    {
        dsn::service::zauto_lock l(_lock);
        int cluster_id;
        std::stringstream ss;
        ss << "SELECT cluster_id from monitor_pegasus_app where app_id = " << resp.app_id << ";";

        sql::ResultSet *res = selectApp(ss);
        if ( !res->next() )
        {
            derror("No next value");
            delete res;
            on_finish();
            return;
        }
        else
        {
            cluster_id = res->getInt(1);
            delete res;
        }

        bool isExist;
        for ( int i = 0; i < resp.partitions.size(); ++i )
        {
            const dsn::replication::partition_configuration& p = resp.partitions[i];

            std::stringstream ss1;
            ss1 << "SELECT COUNT(*) from monitor_pegasus_partition WHERE cluster_id = " << cluster_id << " AND app_id = "
                << resp.app_id << " AND partition_index = " << p.gpid.pidx << ";";
            res = selectApp(ss1);
            if ( !res->next() )
            {
                derror("No next value");
                delete res;
                on_finish();
                return;
            }
            else
                isExist = res->getInt(1) > 0 ? true : false;
            delete res;

            //update the app 'monitor_pegasus_partition'
            std::stringstream ss_partition;
            if ( !isExist )
                ss_partition << "INSERT INTO monitor_pegasus_partition(app_id, partition_index, ballot, last_committed_decree,"
                             << "cluster_id) values('" << resp.app_id << "', '" << p.gpid.pidx
                             << "','" << p.ballot << "','" << p.last_committed_decree << "', '" << cluster_id << "');";
            else
                ss_partition << "UPDATE monitor_pegasus_partition SET ballot = '" << p.ballot << "', last_committed_decree = '" << p.last_committed_decree
                             << "' WHERE cluster_id='" << cluster_id << "' AND app_id=" << resp.app_id << " AND partition_index=" << p.gpid.pidx << ";";
            updateApp(ss_partition);

            if ( !updatePegasusToTask(resp, p, cluster_id) )
            {
                on_finish();
                return;
            }
        }
        on_finish();
    }
}

void info_collector::getHostName(char hn[], const dsn::rpc_address& addr, int len/* = 100*/)
{
    struct sockaddr_in addr2;
    addr2.sin_addr.s_addr = htonl(addr.ip());
    addr2.sin_family = AF_INET;
    if (getnameinfo((struct sockaddr*)&addr2, sizeof(sockaddr), hn, sizeof(hn) * len, nullptr, 0, NI_NAMEREQD))
    {
        inet_ntop(AF_INET, &(addr2.sin_addr), hn, 100);
    }
}

//success:true, else:false
bool info_collector::updatePegasusToTask(configuration_query_by_index_response &resp, const dsn::replication::partition_configuration &p, int cluster_id)
{
    sql::ResultSet *res;
    int task_id;
    int partition_id;
    std::vector<std::string> hostnameVector;
    std::vector<std::uint16_t> portVector;

    //get the partition id in app 'monitor_pegasus_partition'
    std::stringstream ss_partition_id;
    ss_partition_id << "SELECT id FROM monitor_pegasus_partition WHERE app_id=" << resp.app_id << " AND cluster_id=" << cluster_id
                    << " AND partition_index=" << p.gpid.pidx << ";";
    res = selectApp(ss_partition_id);
    if ( !res->next() )
    {
        derror("No next value in \"SELECT id FROM monitor_pegasus_partition ***\"");
        delete res;
        return false;
    }
    else
    {
        partition_id = res->getInt(1);
        delete res;
    }

    //get the hostname of primary
    char hostname_primary[100];
    getHostName(hostname_primary, p.primary);
    hostnameVector.push_back(hostname_primary);
    portVector.push_back(p.primary.port());
    for ( int index = 0; index < p.secondaries.size(); ++index )
    {
        char hostname_secondaries[100];
        getHostName(hostname_secondaries, p.secondaries[index]);
        hostnameVector.push_back(hostname_secondaries);
        portVector.push_back(p.secondaries[index].port());
    }

    std::stringstream ss_for_partitionToTask;
    for ( int index = 0; index < hostnameVector.size(); ++index )
    {
        int type = index ? PS_SECONDARY : PS_PRIMARY;
        std::stringstream ss_for_taskid;
        ss_for_taskid << "SELECT id FROM monitor_task WHERE host='" << hostnameVector[index] << "' AND port=" << portVector[index]
                      << " AND job_id=2;";
        res = selectApp(ss_for_taskid);
        if ( !res->next() )
        {
            derror("No next value in \"SELECT id FROM monitor_task ***\"");
            delete res;
            return false;
        }
        else
        {
            task_id = res->getInt(1);
            delete res;
        }

        //judge if there exists an item than type = PS_PRIMARY AND partition_id='partition_id' AND task_id='task_id'
        std::stringstream ss_count;
        ss_count << "SELECT COUNT(*) FROM monitor_pegasus_partition_to_task WHERE type=" << type << " AND partition_id="
                 << partition_id << " AND task_id=" << task_id << ";";
        res = selectApp(ss_count);
        bool isExist;
        if ( !res->next() )
        {
            derror("No next value in \"SELECT id FROM monitor_task ***\"");
            delete res;
            return false;
        }
        else
        {
            isExist = res->getInt(1) > 0 ? true : false;
            delete res;
        }

        if ( !isExist )
            ss_for_partitionToTask << "INSERT INTO monitor_pegasus_partition_to_task(type, partition_id, task_id) values("
                                   << type << ", " << partition_id << ", " << task_id << ");";
        else
            ss_for_partitionToTask << "UPDATE monitor_pegasus_partition_to_task SET type=" << type << " WHERE partition_id=" << partition_id
                                   << " AND task_id=" << task_id << ";";
    }
    exeTransaction(ss_for_partitionToTask);
    return true;
}


/*
//success:true, else:false
bool info_collector::updatePegasusToTask(configuration_query_by_index_response &resp, const dsn::replication::partition_configuration &p, int cluster_id)
{
    sql::ResultSet *res;
    int task_id;
    int partition_id;
    std::vector<std::string> hostname;

    //get the partition id in app 'monitor_pegasus_partition'
    std::stringstream ss_partition_id;
    ss_partition_id << "SELECT id FROM monitor_pegasus_partition WHERE app_id=" << resp.app_id << " AND cluster_id=" << cluster_id
                    << " AND partition_index=" << p.gpid.pidx << ";";
    res = selectApp(ss_partition_id);
    if ( !res->next() )
    {
        derror("No next value in \"SELECT id FROM monitor_pegasus_partition ***\"");
        delete res;
        return false;
    }
    else
    {
        partition_id = res->getInt(1);
        delete res;
    }

    //get the hostname of primary
    char hostname_primary[100];
    getHostName(hostname_primary, p.primary);
    //get the task id in app 'monitor_task'
    std::stringstream ss_for_taskid;
    ss_for_taskid << "SELECT id FROM monitor_task WHERE host='" << hostname_primary << "' AND port=" << p.primary.port()
                  << " AND job_id=2;";
    res = selectApp(ss_for_taskid);
    if ( !res->next() )
    {
        derror("No next value in \"SELECT id FROM monitor_task ***\"");
        delete res;
        return false;
    }
    else
    {
        task_id = res->getInt(1);
        delete res;
    }


    //judge if there exists an item than type = PS_PRIMARY AND partition_id='partition_id' AND task_id='task_id'
    std::stringstream ss_count;
    ss_count << "SELECT COUNT(*) FROM monitor_pegasus_partition_to_task WHERE type=" << dsn::replication::PS_PRIMARY << " AND partition_id="
             << partition_id << " AND task_id=" << task_id << ";";
    res = selectApp(ss_count);
    bool isExist;
    if ( !res->next() )
    {
        derror("No next value in \"SELECT id FROM monitor_task ***\"");
        delete res;
        return false;
    }
    else
    {
        isExist = res->getInt(1) > 0 ? true : false;
        delete res;
    }

    //delUndesiredInPartitionToTask(resp, , );
    //update partition_to_task where type = PS_PRIMARY
    std::stringstream ss_for_partitionToTask;
    if ( !isExist )
        ss_for_partitionToTask << "INSERT INTO monitor_pegasus_partition_to_task(type, partition_id, task_id) values("
                               << PS_PRIMARY << ", " << partition_id << ", " << task_id << ");";
    else
        ss_for_partitionToTask << "UPDATE monitor_pegasus_partition_to_task SET type=" << PS_PRIMARY << " WHERE partition_id=" << partition_id
                               << " AND task_id=" << task_id << ";";
    updateApp(ss_for_partitionToTask);

    //update monitor_pegasus_partition_to_task where type = PS_SECONDARY
    for ( int index = 0; index < p.secondaries.size(); ++index )
    {
        //get the hostname of secondaries[index]
        char hostname_secondaries[100];
        getHostName(hostname_secondaries, p.secondaries[index]);

        //judge if there has been an item that host='hostname_secondaries' AND port=p.secondaries[index].port() AND job_id=2;
        std::stringstream ss_for_taskid_secondary;
        ss_for_taskid_secondary << "SELECT id FROM monitor_task WHERE host='" << hostname_secondaries << "' AND port=" << p.secondaries[index].port()
                      << " AND job_id=2;";
        res = selectApp(ss_for_taskid_secondary);
        if ( !res->next() )
        {
            derror("No next value in \"SELECT id FROM monitor_task ***\"");
            delete res;
            return false;
        }
        else
        {
            task_id = res->getInt(1);
            delete res;
        }

        std::stringstream ss_count_secondaries;
        ss_count_secondaries << "SELECT COUNT(*) FROM monitor_pegasus_partition_to_task WHERE type=" << dsn::replication::PS_SECONDARY
                             << " AND partition_id=" << partition_id << " AND task_id=" << task_id << ";";
        res = selectApp(ss_count_secondaries);
        bool isExist_secondary;
        if ( !res->next() )
        {
            derror("No next value in \"SELECT id FROM monitor_task ***\"");
            delete res;
            return false;
        }
        else
        {
            isExist_secondary = res->getInt(1) > 0 ? true : false;
            delete res;
        }

        //update monitor_pegasus_partition_to_task secondaries[index]
        std::stringstream ss_for_partitionToTask_secondary;
        if ( !isExist_secondary )
            ss_for_partitionToTask_secondary << "INSERT INTO monitor_pegasus_partition_to_task(type, partition_id, task_id) values("
                                   << PS_SECONDARY << ", " << partition_id << ", " << task_id << ");";
        else
            ss_for_partitionToTask_secondary << "UPDATE monitor_pegasus_partition_to_task SET type=" << PS_SECONDARY
                                             << " WHERE partition_id=" << partition_id << " AND task_id=" << task_id << ";";
        updateApp(ss_for_partitionToTask_secondary);
    }
    return true;
}
*/

// on finish
void info_collector::on_finish()
{
    int count = _running_count.fetch_sub(1);
    //std::cout << "count: " << count  << ";  _running_count: " << _running_count << std::endl;
    if(count == 1)
    {
        delete _con;
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
void info_collector::end_meta_request(task_ptr callback, int retry_times, error_code err, dsn_message_t request, dsn_message_t resp, bool is_retry)
{
    if(err == dsn::ERR_TIMEOUT && is_retry && retry_times < 5)
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
                end_meta_request(std::move(callback_capture), retry_times + 1, err, request, response, is_retry);
            },
            0
         );
    }
    else
        callback->enqueue_rpc_response(err, resp);
}

}} // namespace
