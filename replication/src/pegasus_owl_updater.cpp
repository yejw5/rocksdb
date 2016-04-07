/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 *
 * -=- Robust Distributed System Nucleus (rDSN) -=-
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     profiler for pegasus project
 *     execute all perf counter in this perf counter file
 *
 * Revision history:
 *     2016-03-10, zhangxiaotong3, update with new owl monitor system
 */

#include <dsn/internal/perf_counter.h>
#include "pegasus_owl_updater.h"
#include "pegasus_io_service.h"

using namespace dsn;

namespace pegasus{ namespace tools{

pegasus_owl_updater::pegasus_owl_updater()
{
    _meta.active = dsn_config_get_value_bool("pegasus.owl", "info_active", true, "meta_active");
    _meta.cluster = dsn_config_get_value_string("pegasus.owl", "info_cluster", "", "meta_cluster");
    dassert(_meta.cluster.size() > 0, "");
    //_meta.host = dsn_config_get_value_string("pegasus.owl", "info_host", "", "meta_host");
    //dassert(_meta.host.size() > 0, "");
    // _meta.job = dsn_config_get_value_string("pegasus.owl", "info_job", "", "meta_job");

    //_meta.port = dsn_config_get_value_uint64("pegasus.owl", "info_port", 0, "meta_port");

    dsn_app_info info;
    dsn_get_current_app_info(&info);
    _meta.job = info.name;
    dassert(_meta.job.size() > 0, "");

    _meta.service = dsn_config_get_value_string("pegasus.owl", "info_service", "", "meta_service");
    dassert(_meta.service.size() > 0, "");
    _meta.version = dsn_config_get_value_string("pegasus.owl", "info_version", "", "meta_version");
    dassert(_meta.version.size() > 0, "");

    //char temp[20];
    //sprintf(temp, "%s:%d", _meta.host.c_str(), _meta.port);
    rpc_address addr(dsn_primary_address());
    _task.assign(addr.to_std_string());

    std::unordered_map<std::string, std::string> map;
    map.insert(std::pair<std::string, std::string>("key", "cluster"));
    map.insert(std::pair<std::string, std::string>("value", _meta.cluster));
    _dimensions.push_back(map);
    _dimensions_with_replica.push_back(map);
    map.clear();
    map.insert(std::pair<std::string, std::string>("key", "job"));
    map.insert(std::pair<std::string, std::string>("value", _meta.job));
    _dimensions.push_back(map);
    _dimensions_with_replica.push_back(map);

    map.clear();
    map.insert(std::pair<std::string, std::string>("key", "task"));
    map.insert(std::pair<std::string, std::string>("value", _task));
    _dimensions_with_replica.push_back(map);


    // init http
    _event_base = event_base_new();
    _owl_host = dsn_config_get_value_string("pegasus.owl", "owl_host", "", "owl_host");
    dassert(_owl_host.size() > 0, "");
    _owl_path = dsn_config_get_value_string("pegasus.owl", "owl_path", "", "owl_path");
    dassert(_owl_path.size() > 0, "");
    _owl_port = dsn_config_get_value_uint64("pegasus.owl", "owl_port", 0, "owl_port");
    dassert(_owl_port > 0, "");

    // start update timer
    _owl_update_interval_seconds = dsn_config_get_value_uint64("pegasus.owl", "update_interval_seconds", 10, "update_interval_seconds");
    _timer.reset(new boost::asio::deadline_timer(pegasus_io_service::instance().ios));
    _timer->expires_from_now(boost::posix_time::seconds(rand() % _owl_update_interval_seconds + 1));
    _timer->async_wait(std::bind(&pegasus_owl_updater::on_timer, this, _timer, std::placeholders::_1));
}

pegasus_owl_updater::~pegasus_owl_updater()
{
    _timer->cancel();
}

bool pegasus_owl_updater::register_handler(perf_counter *pc)
{
    dinfo("register owl handler, %s", pc->full_name());
    utils::auto_write_lock l(_lock);
    auto it = _perf_counters.find(pc);
    if (it == _perf_counters.end() )
    {
        //pc->add_ref();
        _perf_counters[pc] = pc->type();
        return true;
    }
    else
    {
        dassert(false, "registration confliction for '%s'", pc->name());
        return false;
    }
}

bool pegasus_owl_updater::unregister_handler(perf_counter *pc)
{
    dinfo("unregister owl handler, %s", pc->full_name());
    utils::auto_write_lock l(_lock);
    auto it = _perf_counters.find(pc);
    if (it != _perf_counters.end() )
    {
        _perf_counters.erase(it);
        //pc->release_ref();
        return true;
    }
    return false;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

void pegasus_owl_updater::update()
{
    dinfo("start owl update");
    if(_flag.test_and_set())
        return;
    perf_counter_map tmp_map;
    {
        utils::auto_read_lock l(_lock);
        for(const std::pair<perf_counter*, dsn_perf_counter_type_t>& kvp : _perf_counters)
        {
            kvp.first->add_ref();
            tmp_map.insert(kvp);
        }
    }
    if(tmp_map.size() == 0)
    {
        dinfo("no owl update needed");
        _flag.clear();
        return;
    }

    owl_report_info info;
    info.meta = _meta;
    for(const std::pair<perf_counter*, dsn_perf_counter_type_t>& kvp : tmp_map)
    {
        owl_metric m;
        m.name.assign(kvp.first->full_name());
        m._namespace = kvp.first->app();
        m.timestamp = dsn_now_ns() / 1000000000;
        switch(kvp.second)
        {
        case COUNTER_TYPE_NUMBER:
            m.unit = "Count";
            m.value = kvp.first->get_integer_value();
            m.dimensions = _dimensions;
            info.metrics.push_back(m);
            m.dimensions = _dimensions_with_replica;
            info.metrics.push_back(m);
            break;
        case COUNTER_TYPE_RATE:
            m.unit = "CountPerSecond";
            m.value = kvp.first->get_integer_value();
            m.dimensions = _dimensions;
            info.metrics.push_back(m);
            m.dimensions = _dimensions_with_replica;
            info.metrics.push_back(m);
            break;
        case COUNTER_TYPE_NUMBER_PERCENTILES:
            m.unit = "Percent";
            m.value = (uint64_t)kvp.first->get_percentile(COUNTER_PERCENTILE_99);
            m.dimensions = _dimensions;
            info.metrics.push_back(m);
            m.dimensions = _dimensions_with_replica;
            info.metrics.push_back(m);
            break;
        default:
            break;
        }
        kvp.first->release_ref();
    }
    std::stringstream ss;
    info.json_state(ss);
    std::string buff = ss.str();
    replaceAll(buff, "_namespace", "namespace");
    replaceAll(buff, "\"active\":1,", "\"active\":true,");

    update_owl(buff);
    _flag.clear();
}

void pegasus_owl_updater::update_owl(std::string buff)
{
    dinfo("start owl update_owl, %s", buff.c_str());
    struct event_base *base = event_base_new();
    struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, _owl_host.c_str(), _owl_port);
    struct evhttp_request *req = evhttp_request_new(pegasus_owl_updater::http_request_done, base);

    evhttp_add_header(req->output_headers, "Host", _owl_host.c_str());
    evhttp_add_header(req->output_headers, "Content-Type", "application/json");
    evbuffer_add(req->output_buffer, buff.c_str(), buff.size());
    //evhttp_connection_set_timeout(req->evcon, 600);
    evhttp_make_request(conn, req, EVHTTP_REQ_POST, _owl_path.c_str());

    event_base_dispatch(base);

    //clear;
    evhttp_connection_free(conn);
    event_base_free(base);
}

void pegasus_owl_updater::http_request_done(struct evhttp_request *req, void *arg)
{
    struct event_base* event = (struct event_base*)arg;
    switch(req->response_code)
    {
    case HTTP_OK:
        {
            dinfo("owl http_request_done OK");
            event_base_loopexit(event, 0);
        }
        break;

    default:
        derror("owl update receive ERROR: %u", req->response_code);

        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        size_t len = evbuffer_get_length(buf);
        char *tmp = (char*)malloc(len+1);
        memcpy(tmp, evbuffer_pullup(buf, -1), len);
        tmp[len] = '\0';
        derror("owl update receive ERROR: %u, %s", req->response_code, tmp);
        free(tmp);
        event_base_loopexit(event, 0);
        return;
    }
}

void pegasus_owl_updater::on_timer(std::shared_ptr<boost::asio::deadline_timer> timer, const boost::system::error_code& ec)
{
    //as the callback is not in tls context, so the log system calls like ddebug, dassert will cause a lock
    if (!ec)
    {
        update();
        timer->expires_from_now(boost::posix_time::seconds(_owl_update_interval_seconds));
        timer->async_wait(std::bind(&pegasus_owl_updater::on_timer, this, timer, std::placeholders::_1));
    }
    else if (boost::system::errc::operation_canceled != ec)
    {
        dassert(false, "pegasus owl updater on_timer error!!!");
    }
}

}} // namespace
