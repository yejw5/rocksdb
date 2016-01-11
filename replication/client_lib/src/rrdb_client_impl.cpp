#include <cctype>
#include <algorithm>
#include <string>

#include <dsn/cpp/auto_codes.h>
#include "rrdb_client_impl.h"
#include "rrdb_error.h"
#include "../../src/rrdb.code.definition.h"
#include "rrdb.client.h"

using namespace dsn;

namespace dsn{ namespace apps{

#define ROCSKDB_ERROR_START -1000

std::unordered_map<int, std::string> rrdb_client_impl::_client_error_to_string;
std::unordered_map<int, int> rrdb_client_impl::_server_error_to_client;

rrdb_client_impl::rrdb_client_impl(const char* app_name, const char* cluster_name, const std::vector< ::dsn::rpc_address>& meta_servers)
    :_app_name(app_name), _cluster(cluster_name), _client(meta_servers, app_name)
{}

const char* rrdb_client_impl::get_cluster_name() const
{
    return _cluster.c_str();
}

const char* rrdb_client_impl::get_app_name() const
{
    return _app_name.c_str();
}

int rrdb_client_impl::set(
        const std::string& hash_key,
        const std::string& sort_key,
        const std::string& value,
        int timeout_milliseconds
        )
{
    // check params
    if(hash_key.empty())
        return ERROR_INVALID_HASH_KEY;

    update_request req;
    generate_key(req.key, hash_key, sort_key);
    req.value.assign(value.c_str(), 0, value.size());
    int resp;
    error_code err = _client.put(req, resp, timeout_milliseconds);
    return get_client_error(err == ERR_OK ? get_rocksdb_server_error(resp) : err.get());
}


int rrdb_client_impl::get(
        const std::string& hash_key,
        const std::string& sort_key,
        std::string& value,
        int timeout_milliseconds
        )
{
    // check params
    if(hash_key.empty())
        return ERROR_INVALID_HASH_KEY;

    dsn::blob req;
    generate_key(req, hash_key, sort_key);
    read_response resp;
    error_code err = _client.get(req, resp, timeout_milliseconds);
    if(err == ERR_OK)
        value.assign(resp.value);
    return get_client_error(err == ERR_OK ? get_rocksdb_server_error(resp.error) : err.get());
}

int rrdb_client_impl::del(
        const std::string& hash_key,
        const std::string& sort_key,
        int timeout_milliseconds
        )
{
    // check params
    if(hash_key.empty())
        return ERROR_INVALID_HASH_KEY;

    dsn::blob req;
    generate_key(req, hash_key, sort_key);
    int resp;
    error_code err = _client.remove(req, resp, timeout_milliseconds);
    return get_client_error(err == ERR_OK ? get_rocksdb_server_error(resp) : err.get());
}

const char* rrdb_client_impl::get_error_string(int error_code) const
{
    auto it = _client_error_to_string.find(error_code);
    dassert(it != _client_error_to_string.end(), "client error %d have no error string", error_code);
    return it->second.c_str();
}

/*static*/ void rrdb_client_impl::init_error()
{
    _client_error_to_string.clear();
    #define ERROR_CODE(x, y, z) _client_error_to_string[y] = z
    #include "rrdb_error_def.h"
    #undef ERROR_CODE

    _server_error_to_client.clear();
    _server_error_to_client[dsn::ERR_OK] = ERROR_OK;
    _server_error_to_client[dsn::ERR_TIMEOUT] = ERROR_TIMEOUT;
    _server_error_to_client[dsn::ERR_FILE_OPERATION_FAILED] = ERROR_SERVER_INTERNAL_ERROR;
    _server_error_to_client[dsn::ERR_OBJECT_NOT_FOUND] = ERROR_OBJECT_NOT_FOUND;

    _server_error_to_client[dsn::replication::ERR_APP_NOT_EXIST] = ERROR_APP_NOT_EXIST;
    _server_error_to_client[dsn::replication::ERR_APP_EXIST] = ERROR_APP_EXIST;

    // rocksdb error;
    for(int i = 1001; i < 1013; i++)
    {
        _server_error_to_client[-i] = -i;
    }
}

/*static*/ int rrdb_client_impl::get_client_error(int server_error)
{
    auto it = _server_error_to_client.find(server_error);
    if(it != _server_error_to_client.end())
        return it->second;
    derror("can't find corresponding client error definition, server error:[%d:%s]", server_error, dsn::error_code(server_error).to_string());
    return ERROR_UNKNOWN;
}

/*static*/ int rrdb_client_impl::get_rocksdb_server_error(int rocskdb_error)
{
    return (rocskdb_error == 0) ? 0 : ROCSKDB_ERROR_START - rocskdb_error;
}

/*static*/ void rrdb_client_impl::generate_key(dsn::blob& key, const std::string& hash_key, const std::string& sort_key)
{
    int len = 4 + hash_key.size() + sort_key.size();
    char* buf = new char[len];
    *(int*)buf = hash_key.size();
    hash_key.copy(buf + 4, hash_key.size(), 0);
    sort_key.copy(buf + 4 + hash_key.size(), sort_key.size(), 0);
    std::shared_ptr<char> buffer(buf);
    key.assign(buffer, 0, len);
}
}} // namespace
