# pragma once
# include "rrdb.client.h"

namespace dsn { namespace apps {  

 
class rrdb_perf_test_client 
    : public rrdb_client, 
      public ::dsn::service::perf_client_helper 
{
public:
    using rrdb_client::rrdb_client;

    void start_test()
    {
        perf_test_suite s;
        std::vector<perf_test_suite> suits;

        s.name = "rrdb.put";
        s.config_section = "task.RPC_RRDB_RRDB_PUT";
        s.send_one = [this](int payload_bytes, int key_space_size){this->send_one_put(payload_bytes, key_space_size); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        s.name = "rrdb.remove";
        s.config_section = "task.RPC_RRDB_RRDB_REMOVE";
        s.send_one = [this](int payload_bytes, int key_space_size){this->send_one_remove(payload_bytes, key_space_size); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        s.name = "rrdb.merge";
        s.config_section = "task.RPC_RRDB_RRDB_MERGE";
        s.send_one = [this](int payload_bytes, int key_space_size){this->send_one_merge(payload_bytes, key_space_size); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        s.name = "rrdb.get";
        s.config_section = "task.RPC_RRDB_RRDB_GET";
        s.send_one = [this](int payload_bytes, int key_space_size){this->send_one_get(payload_bytes, key_space_size); };
        s.cases.clear();
        load_suite_config(s);
        suits.push_back(s);
        
        start(suits);
    }                

    void send_one_put(int payload_bytes, int key_space_size)
    {
        update_request req;
        
        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer(payload_bytes < 128 ? 128 : payload_bytes);
        writer.write("key.", 4);
        writer.write(rs);
        req.key = writer.get_buffer();

        while (writer.total_size() < payload_bytes)
            writer.write(req.key);
        req.value = writer.get_buffer();
        
        put(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }

    void send_one_remove(int payload_bytes, int key_space_size)
    {
        ::dsn::blob req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer;
        writer.write("key.", 4);
        writer.write(rs);
        req = writer.get_buffer();
                
        remove(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }

    void send_one_merge(int payload_bytes, int key_space_size)
    {
        update_request req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer(payload_bytes < 128 ? 128 : payload_bytes);
        writer.write("key.", 4);
        writer.write(rs);
        req.key = writer.get_buffer();

        while (writer.total_size() < payload_bytes)
            writer.write(req.key);
        req.value = writer.get_buffer();

        merge(
            req,
            [this, context = prepare_send_one()](error_code err, int&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }

    void send_one_get(int payload_bytes, int key_space_size)
    {
        ::dsn::blob req;

        auto rs = random64(0, 10000000) % key_space_size;
        binary_writer writer;
        writer.write("key.", 4);
        writer.write(rs);
        req = writer.get_buffer();
        
        get(
            req,
            [this, context = prepare_send_one()](error_code err, read_response&& resp)
            {
                end_send_one(context, err);
            },
            _timeout
            );
    }
};

} } 
