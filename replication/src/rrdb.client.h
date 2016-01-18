# pragma once
# include <dsn/dist/replication.h>
# include "rrdb.code.definition.h"
# include <iostream>

namespace dsn { namespace apps { 
class rrdb_client 
    : public ::dsn::replication::replication_app_client_base
{
public:
    rrdb_client(
        const std::vector< ::dsn::rpc_address>& meta_servers,
        const char* replicate_app_name)
        : ::dsn::replication::replication_app_client_base(meta_servers, replicate_app_name)
    {
    }
    
    virtual ~rrdb_client() {}
    
    // from requests to partition index
    // PLEASE DO RE-DEFINE THEM IN A SUB CLASS!!!
    virtual uint64_t get_key_hash(const update_request& key)
    {
        return dsn_crc64_compute(key.key.data(), key.key.length(), 0);
    }

    virtual uint64_t get_key_hash(const ::dsn::blob& key)
    {
        return dsn_crc64_compute(key.data(), key.length(), 0);
    }

    // ---------- call RPC_RRDB_RRDB_PUT ------------
    // - synchronous 
    ::dsn::error_code put(
        const update_request& update, 
        /*out*/ int& resp, 
        int timeout_milliseconds = 0 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::write(
            get_key_hash(update),
            RPC_RRDB_RRDB_PUT,
            update,
            nullptr,
            timeout_milliseconds,
            0 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack update_request and int 
    ::dsn::task_ptr begin_put(
        const update_request& update,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0 
        )
    {
        return ::dsn::replication::replication_app_client_base::write(
            get_key_hash(update),
            RPC_RRDB_RRDB_PUT, 
            update,
            this,
            [=](error_code err, int resp)
            {
                end_put(err, resp, context);
            },
            timeout_milliseconds,
            reply_hash 
            );
    }

    virtual void end_put(
        ::dsn::error_code err, 
        const int& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_RRDB_RRDB_PUT err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_RRDB_RRDB_PUT ok" << std::endl;
        }
    }
    
    // ---------- call RPC_RRDB_RRDB_REMOVE ------------
    // - synchronous 
    ::dsn::error_code remove(
        const ::dsn::blob& key, 
        /*out*/ int& resp, 
        int timeout_milliseconds = 0 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::write(
            get_key_hash(key),
            RPC_RRDB_RRDB_REMOVE,
            key,
            nullptr,
            timeout_milliseconds,
            0 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack ::dsn::blob and int 
    ::dsn::task_ptr begin_remove(
        const ::dsn::blob& key,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0 
        )
    {
        return ::dsn::replication::replication_app_client_base::write(
            get_key_hash(key),
            RPC_RRDB_RRDB_REMOVE, 
            key,
            this,
            [=](error_code err, int resp)
            {
                end_remove(err, resp, context);
            },
            timeout_milliseconds,
            reply_hash 
            );
    }

    virtual void end_remove(
        ::dsn::error_code err, 
        const int& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_RRDB_RRDB_REMOVE err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_RRDB_RRDB_REMOVE ok" << std::endl;
        }
    }

    // ---------- call RPC_RRDB_RRDB_MERGE ------------
    // - synchronous 
    ::dsn::error_code merge(
        const update_request& update, 
        /*out*/ int& resp, 
        int timeout_milliseconds = 0 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::write(
            get_key_hash(update),
            RPC_RRDB_RRDB_MERGE,
            update,
            nullptr,
            timeout_milliseconds,
            0 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack update_request and int 
    ::dsn::task_ptr begin_merge(
        const update_request& update,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0 
        )
    {
        return ::dsn::replication::replication_app_client_base::write(
            get_key_hash(update),
            RPC_RRDB_RRDB_MERGE, 
            update,
            this,
            [=](error_code err, int resp)
            {
                end_merge(err, resp, context);
            },
            timeout_milliseconds,
            reply_hash 
            );
    }

    virtual void end_merge(
        ::dsn::error_code err, 
        const int& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_RRDB_RRDB_MERGE err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_RRDB_RRDB_MERGE ok" << std::endl;
        }
    }
    
    // ---------- call RPC_RRDB_RRDB_GET ------------
    // - synchronous 
    ::dsn::error_code get(
        const ::dsn::blob& key, 
        /*out*/ read_response& resp, 
        int timeout_milliseconds = 0, 
        ::dsn::replication::read_semantic_t read_semantic = ::dsn::replication::read_semantic_t::ReadLastUpdate 
        )
    {
        auto resp_task = ::dsn::replication::replication_app_client_base::read(
            get_key_hash(key),
            RPC_RRDB_RRDB_GET,
            key,
            nullptr,
            timeout_milliseconds,
            0, 
            read_semantic 
            );
        resp_task->wait();
        if (resp_task->error() == ::dsn::ERR_OK)
        {
            ::unmarshall(resp_task->response(), resp);
        }
        return resp_task->error();
    }
    
    // - asynchronous with on-stack ::dsn::blob and read_response 
    ::dsn::task_ptr begin_get(
        const ::dsn::blob& key,
        void* context = nullptr,
        int timeout_milliseconds = 0, 
        int reply_hash = 0,  
        ::dsn::replication::read_semantic_t read_semantic = ::dsn::replication::read_semantic_t::ReadLastUpdate 
        )
    {
        return ::dsn::replication::replication_app_client_base::read(
            get_key_hash(key),
            RPC_RRDB_RRDB_GET, 
            key,
            this,
            [=](error_code err, read_response&& resp)
            {
                end_get(err, resp, context);
            },
            timeout_milliseconds,
            reply_hash, 
            read_semantic 
            );
    }

    virtual void end_get(
        ::dsn::error_code err, 
        const read_response& resp,
        void* context)
    {
        if (err != ::dsn::ERR_OK) std::cout << "reply RPC_RRDB_RRDB_GET err : " << err.to_string() << std::endl;
        else
        {
            std::cout << "reply RPC_RRDB_RRDB_GET ok" << std::endl;
        }
    }
};

} } 
