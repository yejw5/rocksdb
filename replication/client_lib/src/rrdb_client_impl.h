#pragma once
#include <string>
#include "rrdb_client.h"
#include "rrdb.client.h"

using namespace dsn::replication;

namespace dsn{ namespace apps{

class rrdb_client_impl : public irrdb_client
{
public:
    rrdb_client_impl(const char* app_name, const char* cluster_name, const std::vector< ::dsn::rpc_address>& meta_servers);
    virtual ~rrdb_client_impl(){}

    virtual const char* get_cluster_name() const override;

    virtual const char* get_app_name() const override;

    virtual int set(
        const std::string& hashkey,
        const std::string& sortkey,
        const std::string& value,
        int timeout_milliseconds = 5000
        ) override;

    virtual int get(
        const std::string& hashkey,
        const std::string& sortkey,
        std::string& value,
        int timeout_milliseconds = 5000
        ) override;

    virtual int del(
        const std::string& hashkey,
        const std::string& sortkey,
        int timeout_milliseconds = 5000
        ) override;

    virtual const char* get_error_string(int error_code) const override;

    static void init_error();

private:
    static uint64_t get_key_hash(const dsn::blob& hash_key);
    static void generate_key(dsn::blob& key, const std::string& hash_key, const std::string& sort_key);
    static bool valid_app_char(const int c);
    static int get_client_error(dsn::error_code server_error);

private:
    rrdb_client _client;
    std::string _cluster;
    std::string _app_name;

    static std::unordered_map<int, std::string> _client_error_to_string;
    static std::unordered_map<int, int> _server_error_to_client;
};

}} //namespace
