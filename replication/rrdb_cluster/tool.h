//for the auxiliary of main function

#include <iostream>
#include <stdio.h>
#include <string>
#include <rrdb_client.h>
#include "client_ddl.h"
#include <iomanip>

using namespace ::dsn::apps;

static const char* LIST_APP_OP = "app";
static const char* LIST_APPS_OP = "ls";
static const char* LIST_NODES_OP = "nodes";
static const char* CREATE_APP_OP = "create";
static const char* DROP_APP_OP = "drop";
static const char* STOP_MIGRATION_OP = "stop_migration";
static const char* START_MIGRATION_OP = "start_migration";
static const char* BALANCER_OP = "balancer";
static const char* USE_OP = "use";
static const char* SET_OP = "set";
static const char* GET_OP = "get";
static const char* DEL_OP = "del";
static const char* EXIT_OP = "exit";
static const char* HELP_OP = "help";

//using namespace std;

void printHead()
//Print head message
{

    std::cout << "rrdb_cluster1.0 (March 18 2016)" << std::endl;
    std::cout << "Type \"help\" for more information." << std::endl;
}

void printHelpInfo()
//Help information
{
    std::cout << "Usage:" << std::endl;
    std::cout << "\t" << "create:          create -name <app_name> -type <app_type> [-pc partition_count] [-rc replication_count]" << std::endl;
    std::cout << "\t" << "drop:            drop -name <app_name>" << std::endl;
    std::cout << "\t" << "list_apps:       ls [-status <all|available|creating|creating_failed|dropping|dropping_failed|dropped>] [-o <out_file>]" << std::endl;
    std::cout << "\t" << "list_app:        app -name <app_name> [-detailed] [-o <out_file>]" << std::endl;
    std::cout << "\t" << "list_nodes:      nodes [-status <all|alive|unalive>] [-o <out_file>]" << std::endl;
    std::cout << "\t" << "stop_migration:  stop_migration" << std::endl;
    std::cout << "\t" << "start_migration: start_migration" << std::endl;
    std::cout << "\t" << "balancer:        balancer -gpid <appid.pidx> -type <move_pri|copy_pri|copy_sec> -from <from_address> -to <to_address>" << std::endl;
    std::cout << "\t" << "use:             use [table-name]" << std::endl;
    std::cout << "\t" << "set:             set <hash-key> <sort-key> <value>" << std::endl;
    std::cout << "\t" << "get:             get <hash-key> <sort-key>" << std::endl;
    std::cout << "\t" << "del:             del <hash-key> <sort-key>" << std::endl;
    std::cout << "\t" << "exit:            exit" << std::endl;
    std::cout << "\t" << "help:            help" << std::endl;
    std::cout << std::endl;
    std::cout << "\tpartition count must be a power of 2" << std::endl;
    std::cout << "\tapp_name and app_type shoud be composed of a-z, 0-9 and underscore" << std::endl;
    std::cout << "\twithout -o option, program will print status on screen" << std::endl;
    std::cout << "\twith -detailed option, program will also print partition state" << std::endl;
}

bool scanfWord(std::string &str, char endFlag = ' ')
{
    char ch;
    bool commandEnd = true;

    if ( endFlag == '\'')
        while ( (ch = getchar()) != endFlag )
            str += ch;
    else if ( endFlag =='\"' )
    {
        while ( (ch = getchar()) != endFlag )
        {
            if ( ch == '\\' )
                ch = getchar();
            str += ch;
        }
    }
    else
    {
        ch = getchar();
        while ( !(ch == ' ' || ch == '\n') )
        {
            str += ch;
            ch = getchar();
        }
    }
    if ( ch == '\n' )
         return commandEnd;
    else
        return !commandEnd;
}

void scanfCommand(int &Argc, std::string Argv[], int paraNum)
{
    char ch;
    int index;

    //while ( (ch = getchar()) == ' ' );

    for ( index = 0; index < paraNum; ++index )
    {
        while ( (ch = getchar()) == ' ' );

        if ( ch == '\'' )
            scanfWord(Argv[index], '\'');
        else if ( ch == '\"' )
            scanfWord(Argv[index], '\"');
        else if ( ch == '\n' )
            return ;
        else
        {
            Argv[index] = ch;
            if ( scanfWord(Argv[index], ' ') )
            {
                Argc++;
                return;
            }
        }

        Argc++;
    }
    while ( (ch = getchar()) != '\n' );
}

void help_op(int Argc)
{
    if ( Argc == 1 )
        printHelpInfo();
    else
        std::cout << "USAGE: help" << std::endl;
}

void create_app_op(std::string app_name, std::string app_type, int partition_count, int replica_count, dsn::replication::client_ddl& client_of_dsn)
{
    std::cout << "[Parameters]" << std::endl;
    if ( !app_name.empty() )
        std::cout << "app_name: " << app_name << std::endl;
    if ( !app_type.empty() )
        std::cout << "app_type: " << app_type << std::endl;
    if ( partition_count )
        std::cout << "partition_count: " << partition_count << std::endl;
    if ( replica_count )
        std::cout << "replica_count: " << replica_count << std::endl;
    std::cout << std::endl << "[Result]" << std::endl;

    if(app_name.empty() || app_type.empty())
        std::cout << "create -name <app_name> -type <app_type> [-pc partition_count] [-rc replication_count]" << std::endl;
    dsn::error_code err = client_of_dsn.create_app(app_name, app_type, partition_count, replica_count);
    if(err == dsn::ERR_OK)
        std::cout << "create app:" << app_name << " succeed" << std::endl;
    else
        std::cout << "create app:" << app_name << " failed, error=" << dsn_error_to_string(err) << std::endl;
}

void drop_app_op(std::string app_name, dsn::replication::client_ddl &client_of_dsn)
{
    if(app_name.empty())
        std::cout << "drop -name <app_name>" << std::endl;
    dsn::error_code err = client_of_dsn.drop_app(app_name);
    if(err == dsn::ERR_OK)
        std::cout << "drop app:" << app_name << " succeed" << std::endl;
    else
        std::cout << "drop app:" << app_name << " failed, error=" << dsn_error_to_string(err) << std::endl;
}

void list_apps_op(std::string status, std::string out_file, dsn::replication::client_ddl &client_of_dsn)
{
    if ( !(status.empty() && out_file.empty()) )
    {
        std::cout << "[Parameters]" << std::endl;
        if ( !status.empty() )
            std::cout << "status: " << status << std::endl;
        if ( !out_file.empty() )
            std::cout << "out_file: " << out_file << std::endl;
        std::cout << std::endl << "[Result]" << std::endl;
    }

    dsn::replication::app_status s = dsn::replication::AS_INVALID;
    if (!status.empty() && status != "all") {
        std::transform(status.begin(), status.end(), status.begin(), ::toupper);
        status = "AS_" + status;
        s = enum_from_string(status.c_str(), dsn::replication::AS_INVALID);
        if(s == dsn::replication::AS_INVALID)
            std::cout << "list_apps [-status <all|available|creating|creating_failed|dropping|dropping_failed|dropped>] [-o <out_file>]" << std::endl;
    }
    dsn::error_code err = client_of_dsn.list_apps(s, out_file);
    if(err == dsn::ERR_OK)
        std::cout << "list apps succeed" << std::endl;
    else
        std::cout << "list apps failed, error=" << dsn_error_to_string(err) << std::endl;
}

void list_app_op(std::string app_name, bool detailed, std::string out_file, dsn::replication::client_ddl &client_of_dsn)
{
    if ( !(app_name.empty() && out_file.empty()) )
    {
        std::cout << "[Parameters]" << std::endl;
        if ( !app_name.empty() )
            std::cout << "app_name: " << app_name << std::endl;
        if ( !out_file.empty() )
            std::cout << "out_file: " << out_file << std::endl;
    }
    if ( detailed )
        std::cout << "detailed: true" << std::endl;
    else
        std::cout << "detailed: false" << std::endl;
    std::cout << std::endl << "[Result]" << std::endl;

    if(app_name.empty())
        std::cout << "list_app -name <app_name> [-detailed] [-o <out_file>]" << std::endl;
    dsn::error_code err = client_of_dsn.list_app(app_name, detailed, out_file);
    if(err == dsn::ERR_OK)
        std::cout << "list app:" << app_name << " succeed" << std::endl;
    else
        std::cout << "list app:" << app_name << " failed, error=" << dsn_error_to_string(err) << std::endl;
}

void list_node_op(std::string status, std::string out_file, dsn::replication::client_ddl &client_of_dsn)
{
    if ( !(status.empty() && out_file.empty()) )
    {
        std::cout << "[Parameters]" << std::endl;
        if ( !status.empty() )
            std::cout << "status: " << status << std::endl;
        if ( !out_file.empty() )
            std::cout << "out_file: " << out_file << std::endl;
        std::cout << std::endl << "[Result]" << std::endl;
    }

    dsn::replication::node_status s = dsn::replication::NS_INVALID;
    if (!status.empty() && status != "all") {
        std::transform(status.begin(), status.end(), status.begin(), ::toupper);
        status = "NS_" + status;
        s = enum_from_string(status.c_str(), dsn::replication::NS_INVALID);
        if(s == dsn::replication::NS_INVALID)
            std::cout << "list_nodes [-status <all|alive|unalive>] [-o <out_file>]" << std::endl;
    }
    dsn::error_code err = client_of_dsn.list_nodes(s, out_file);
    if(err != dsn::ERR_OK)
        std::cout << "list nodes failed, error=" << dsn_error_to_string(err) << std::endl;
}

void stop_migration_op(dsn::replication::client_ddl &client_of_dsn)
{
    dsn::error_code err = client_of_dsn.control_meta_balancer_migration(false);
    std::cout << "stop migration result: " << dsn_error_to_string(err) << std::endl;
}

void start_migration_op(dsn::replication::client_ddl &client_of_dsn)
{
    dsn::error_code err = client_of_dsn.control_meta_balancer_migration(true);
    std::cout << "start migration result: " << err.to_string() << std::endl;
}

void use_op(int Argc, std::string Argv[], std::string &table_name, dsn::replication::configuration_list_apps_response resp)
{
    if ( Argc == 1 && table_name.empty())
        std::cout << "No table is using now!" << std::endl;
    else if ( Argc == 1 && !table_name.empty() )
        std::cout << "The using table is: " << table_name << std::endl;
    else if ( Argc == 2 )
    {
        bool isInResp = false;
        for(int i = 0; i < resp.infos.size(); i++)
            if ( resp.infos[i].app_name == Argv[1] && resp.infos[i].status == dsn::replication::AS_AVAILABLE )
                isInResp = true;
        if ( isInResp )
        {
            table_name = Argv[1];
            std::cout << "Successfully!" << std::endl;
        }
        else
            std::cout << "The table " << Argv[1] << " does\'t exist." << std::endl;
    }
    else
        std::cout << "USAGE: use [table-name]" << std::endl;
}

void get_op(int Argc, std::string Argv[], irrdb_client* client)
{
    if ( Argc != 3 )
    {
        std::cout << "USAGE: set <hash-key> <sort-key>" << std::endl;
        return;
    }

    std::string hash_key = Argv[1];
    std::string sort_key = Argv[2];
    std::string value;
    int ret = client->get(hash_key, sort_key, value);
    if (ret != ERROR_OK) {
        if (ret == ERROR_NOT_FOUND) {
            fprintf(stderr, "Not found\n");
        }
        else {
            fprintf(stderr, "ERROR: %s\n", client->get_error_string(ret));
        }
    }
    else {
        fprintf(stderr, "%s\n", value.c_str());
    }
}

void set_op(int Argc, std::string Argv[], irrdb_client* client)
{
    if ( Argc != 4 )
    {
        std::cout << "USAGE: set <hash-key> <sort-key> <value>" << std::endl;
        return;
    }

    std::string hash_key = Argv[1];
    std::string sort_key = Argv[2];
    std::string value = Argv[3];
    int ret = client->set(hash_key, sort_key, value);
    if (ret != ERROR_OK) {
        fprintf(stderr, "ERROR: %s\n", client->get_error_string(ret));
    }
    else {
        fprintf(stderr, "OK\n");
    }
}

void del_op(int Argc, std::string Argv[], irrdb_client* client)
{
    if ( Argc != 3 )
    {
        std::cout << "USAGE: del <hash-key> <sort-key>" << std::endl;
        return;
    }

    std::string hash_key = Argv[1];
    std::string sort_key = Argv[2];
    int ret = client->del(hash_key, sort_key);
    if (ret != ERROR_OK) {
        fprintf(stderr, "ERROR: %s\n", client->get_error_string(ret));
    }
    else {
        fprintf(stderr, "OK\n");
    }
}
