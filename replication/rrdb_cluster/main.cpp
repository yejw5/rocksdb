// Copyright (c) 2016 xiaomi.com, Inc. All Rights Reserved.

#include <stdio.h>
#include <iostream>
#include "tool.h"
#include <string>
#include <vector>

using namespace ::dsn::apps;

#define PARA_NUM 10

int main()
{
    printHead();
    printHelpInfo();

    if (!rrdb_client_factory::initialize("config.ini")) {
        fprintf(stderr, "ERROR: init pegasus failed\n");
        return -1;
    }

    std::string op_name;
    std::string table_name = "";

    while ( true )
    {
        std::cout << std::endl << ">>> ";

        int Argc = 0;
        std::string Argv[PARA_NUM];
        scanfCommand(Argc, Argv, PARA_NUM);

        if ( Argc == 0 )
            continue;

        std::vector<dsn::rpc_address> meta_servers;
        dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);
        dsn::replication::replication_ddl_client client_of_dsn(meta_servers);
        dsn::replication::configuration_list_apps_response resp;

        std::string app_name;
        std::string app_type;
        int partition_count = 4;
        int replica_count = 3;
        std::string status;
        bool detailed = false;
        std::string out_file;

        for(int index = 1; index < Argc; index++)
        {
            if( Argv[index] == "-name" && Argc > index + 1)
                app_name = Argv[++index];
            else if(Argv[index] == "-type" && Argc > index + 1)
                app_type = Argv[++index];
            else if(Argv[index] == "-pc" && Argc > index + 1)
                partition_count = atol(Argv[++index].c_str());
            else if(Argv[index] == "-rc" && Argc > index + 1)
                replica_count = atol(Argv[++index].c_str());
            else if(Argv[index] == "-status" && Argc > index + 1)
                status = Argv[++index];
            else if(Argv[index] == "-detailed")
                detailed = true;
            else if(Argv[index] == "-o"&& Argc > index + 1)
                out_file = Argv[++index];
        }

        std::string status_for_get_apps;
        dsn::replication::app_status s = dsn::replication::AS_INVALID;
        if (!status_for_get_apps.empty() && status_for_get_apps != "all") {
            std::transform(status_for_get_apps.begin(), status_for_get_apps.end(), status_for_get_apps.begin(), ::toupper);
            status_for_get_apps = "AS_" + status_for_get_apps;
            s = enum_from_string(status_for_get_apps.c_str(), dsn::replication::AS_INVALID);
        }
        //get all apps, for determining whether a table exists.
        //dsn::error_code err = client_of_dsn.get_apps(resp, s, out_file);

        irrdb_client* client_of_rrdb = rrdb_client_factory::get_client(table_name.c_str());

        op_name = Argv[0];
        if ( op_name == HELP_OP )
            help_op(Argc);
        else if ( op_name == CREATE_APP_OP )
        {
            if ( Argc >= 5 )
                create_app_op(app_name, app_type, partition_count, replica_count, client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "create -name <app_name> -type <app_type> [-pc partition_count] [-rc replication_count]" << std::endl;
        }
        else if( op_name == DROP_APP_OP )
        {
            if ( Argc == 3 )
                drop_app_op(app_name, client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "drop -name <app_name>" << std::endl;
        }
        else if ( op_name == LIST_APPS_OP )
        {
            if ( Argc == 1 || (Argc == 3 && (Argv[1] == "-status" || Argv[1] == "-o"))
                 || (Argc == 5 && Argv[1] == "-status" && Argv[3] == "-o") )
                list_apps_op(status, out_file, client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "ls [-status <all|available|creating|creating_failed|dropping|dropping_failed|dropped>] [-o <out_file>]" << std::endl;
        }
        else if ( op_name == LIST_APP_OP )
        {
            if ( Argc >= 3 && Argc <= 6 )
                list_app_op(app_name, detailed, out_file, client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "app -name <app_name> [-detailed] [-o <out_file>]" << std::endl;
        }
        else if ( op_name == LIST_NODES_OP) {
            if ( Argc == 1 || Argc == 3 || Argc == 5 )
                list_node_op(status, out_file, client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "nodes [-status <all|alive|unalive>] [-o <out_file>]" << std::endl;
        }
        else if (op_name == STOP_MIGRATION_OP) {
            if ( Argc == 1 )
                stop_migration_op(client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "stop_migration" << std::endl;
        }
        else if (op_name == START_MIGRATION_OP) {
            if ( Argc == 1 )
                start_migration_op(client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "start_migration" << std::endl;
        }
        else if (op_name == BALANCER_OP) {
            dsn::replication::balancer_proposal_request request;
            for (int i=1; i<Argc-1; i+=2) {
                if ( Argv[i] == "-gpid" ){
                    sscanf(Argv[i+1].c_str(), "%d.%d", &request.gpid.app_id, &request.gpid.pidx);
                }
                else if ( Argv[i] == "-type" ){
                    std::map<std::string, dsn::replication::balancer_type> mapper = {
                        {"move_pri", dsn::replication::BT_MOVE_PRIMARY},
                        {"copy_pri", dsn::replication::BT_COPY_PRIMARY},
                        {"copy_sec", dsn::replication::BT_COPY_SECONDARY}
                    };
                    if (mapper.find(Argv[i+1]) == mapper.end()) {
                        std::cout << "balancer -gpid <appid.pidx> -type <move_pri|copy_pri|copy_sec> -from <from_address> -to <to_address>" << std::endl;
                    }
                    request.type = mapper[Argv[i+1]];
                }
                else if ( Argv[i] == "-from" ) {
                    request.from_addr.from_string_ipv4(Argv[i+1].c_str());
                }
                else if ( Argv[i] == "-to" ) {
                    request.to_addr.from_string_ipv4(Argv[i+1].c_str());
                }
            }
            dsn::error_code err = client_of_dsn.send_balancer_proposal(request);
            std::cout << "send balancer proposal result: " << err.to_string() << std::endl;
        }
        else if ( op_name == USE_OP )
            use_op(Argc, Argv, table_name, resp);
        else if ( op_name == GET_OP )
            get_op(Argc, Argv, client_of_rrdb);
        else if (op_name == SET_OP)
            set_op(Argc, Argv, client_of_rrdb);
        else if ( op_name == DEL_OP )
            del_op(Argc, Argv, client_of_rrdb);
        else if ( op_name == EXIT_OP )
            return 0;
        else
            printf("ERROR: Invalid op-name: %s\n", op_name.c_str());
    }

    return 0;
}
