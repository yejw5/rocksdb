#pragma once

#include <string>
#include "rrdb_error.h"

namespace dsn { namespace apps {

///
/// \brief The irrdb_client class
/// irrdb_client is the base class that users use to access a specific cluster with an app name
/// the class of client provides the basic operation to:
/// set/get/delete the value of a key in a app.
///
class irrdb_client
{
public:
    // destructor
    virtual ~irrdb_client(){}

    ///
    /// \brief get_cluster_name
    /// a cluster is a set of physical computers that used to form a k-v system.
    /// we indicate the cluster using a cluster name.
    /// \return
    ///
    virtual const char* get_cluster_name() const = 0;

    ///
    /// \brief get_app_name
    /// an app is a logical isolated k-v store.
    /// a cluster can have multiple apps.
    /// \return
    ///
    virtual const char* get_app_name() const = 0;

    ///
    /// \brief set
    ///     store the k-v to the cluster.
    ///     key is composed of hashkey and sortkey.
    /// \param hashkey
    /// length must be greater than 1, used to decide which partition to put this k-v
    /// \param sortkey
    /// all the k-v under hashkey will be sorted by sortkey.
    /// \param value
    /// the value we want to store.
    /// \param timeout_milliseconds
    /// if wait longer than this value, will return time out error
    /// \return
    /// int, the error indicates whether or not the operation is succeeded.
    /// this error can be converted to a string using get_error_string()
    ///
    virtual int set(
        const std::string& hashkey,
        const std::string& sortkey,
        const std::string& value,
        int timeout_milliseconds = 5000
        ) = 0;

    ///
    /// \brief get
    ///     get value by key from the cluster.
    ///     key is composed of hashkey and sortkey. must provide both to get the value.
    /// \param hashkey
    /// length must be greater than 1, used to decide which partition to get this k-v
    /// \param sortkey
    /// all the k-v under hashkey will be sorted by sortkey.
    /// \param value
    /// the returned value will be put into it.
    /// \param timeout_milliseconds
    /// if wait longer than this value, will return time out error
    /// \return
    /// int, the error indicates whether or not the operation is succeeded.
    /// this error can be converted to a string using get_error_string()
    ///
    virtual int get(
        const std::string& hashkey,
        const std::string& sortkey,
        std::string& value,
        int timeout_milliseconds = 5000
        ) = 0;

    ///
    /// \brief del
    ///     del stored k-v by key from cluster
    ///     key is composed of hashkey and sortkey. must provide both to get the value.
    /// \param hashkey
    /// length must be greater than 1, used to decide from which partition to del this k-v
    /// \param sortkey
    /// all the k-v under hashkey will be sorted by sortkey.
    /// \param timeout_milliseconds
    /// if wait longer than this value, will return time out error
    /// \return
    /// int, the error indicates whether or not the operation is succeeded.
    /// this error can be converted to a string using get_error_string()
    ///
    virtual int del(
        const std::string& hashkey,
        const std::string& sortkey,
        int timeout_milliseconds = 5000
        ) = 0;

    ///
    /// \brief get_error_string
    /// get error string
    /// all the function above return an int value that indicates an error can be converted into a string for human reading.
    /// \param error_code
    /// all the error code are defined in "rrdb_err_def.h"
    /// \return
    ///
    virtual const char* get_error_string(int error_code) const = 0;
};

class rrdb_client_factory
{
public:
    ///
    /// \brief initialize
    /// initialize rrdb client lib. must call this function before anything else.
    /// \param config_file
    /// the configuration file of rrdb client lib
    /// \return
    /// true indicate the initailize is success.
    ///
    static bool initialize(const char* config_file);

    ///
    /// \brief get_client
    /// get an instance for a given cluster and a given app name.
    /// \param app_name
    /// an app is a logical isolated k-v store.
    /// a cluster can have multiple apps.
    /// \param cluster_name
    /// a cluster is a set of physical computers that used to form a k-v system.
    /// we indicate the cluster using a cluster name.
    /// \return
    /// the client instance. DO NOT delete this client even after usage.
    static irrdb_client* get_client(const char* app_name, const char* cluster_name = "");
};

}} //namespace
