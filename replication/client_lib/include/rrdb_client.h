#pragma once

#include <string>
#include "rrdb_error.h"

namespace dsn { namespace apps {

// irrdb_client is the base class that users use to access a specific cluster.
// user can't create an instance, instead, they should get an instace through client_factory::get_context(const char* cluster_name),
// and destroy through client_factory::drop_context(client* context);
//
// the class of client provides the basic operation to:
// set/get/delete the value of a key in a app.

class irrdb_client
{
public:
    // destructor
    virtual ~irrdb_client(){}

    virtual const char* get_cluster_name() const = 0;
    virtual const char* get_app_name() const = 0;

    virtual int set(
        const std::string& hashkey,
        const std::string& sortkey,
        const std::string& value,
        int timeout_milliseconds = 5000
        ) = 0;

    virtual int get(
        const std::string& hashkey,
        const std::string& sortkey,
        std::string& value,
        int timeout_milliseconds = 5000
        ) = 0;

    virtual int del(
        const std::string& hashkey,
        const std::string& sortkey,
        int timeout_milliseconds = 5000
        ) = 0;

    virtual const char* get_error_string(int error_code) const = 0;
};

class rrdb_client_factory
{
public:
  static bool initialize(const char* config_file);
  static irrdb_client* get_client(const char* app_name, const char* cluster_name = "");
};

}} //namespace
