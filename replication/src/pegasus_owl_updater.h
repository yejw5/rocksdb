#pragma once

#include <dsn/internal/singleton.h>
#include <dsn/internal/synchronize.h>
#include <dsn/c/api_utilities.h>
#include <dsn/internal/perf_counter.h>
#include <dsn/cpp/json_helper.h>


#include <boost/asio.hpp>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>

using namespace dsn;
using namespace dsn::utils;

namespace pegasus{ namespace tools{

struct owl_meta_info{
    std::string cluster;
    std::string job;
    int32_t     port;
    std::string service;
    std::string host;
    bool        active;
    std::string version;

    DEFINE_JSON_SERIALIZATION(cluster, job, port, service, host, active, version)
};

struct owl_metric
{
    std::string unit;
    std::string _namespace;
    std::string name;
    double value;
    int64_t timestamp;
    std::vector<std::unordered_map<std::string, std::string>> dimensions;
    DEFINE_JSON_SERIALIZATION(unit, _namespace, name, value, timestamp, dimensions)
};

struct owl_report_info
{
    owl_meta_info meta;
    std::vector<owl_metric> metrics;
    DEFINE_JSON_SERIALIZATION(meta, metrics)
};

class pegasus_owl_updater : public utils::singleton<pegasus_owl_updater>{
public:
    pegasus_owl_updater();
    virtual ~pegasus_owl_updater();
    bool register_handler(perf_counter* pc);

private:
    void update();
    void update_owl(std::string buff);
    void on_timer(std::shared_ptr<boost::asio::deadline_timer> timer, const boost::system::error_code& ec);
    static void http_request_done(struct evhttp_request *req, void *arg);

    typedef std::unordered_map<perf_counter*, dsn_perf_counter_type_t> perf_counter_map;
    perf_counter_map _perf_counters;
    mutable utils::rw_lock_nr _lock;

    uint32_t _owl_update_interval_seconds;
    std::shared_ptr<boost::asio::deadline_timer> _timer;

    owl_meta_info _meta;
    std::string _task;
    std::vector<std::unordered_map<std::string, std::string>> _dimensions;

    std::atomic_flag _flag;
    struct event_base* _event_base;
    std::string _owl_host;
    std::string _owl_path;
    uint16_t _owl_port;
};

}} // namespace
