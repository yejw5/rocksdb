#include "client_ddl.h"
#include <iostream>

using namespace dsn::replication;



int init_environment(char* exe, char* config_file)
{
    dsn_core_init();

    //use config file to run
    char* argv[] = {exe, config_file};

    dsn_run(2, argv, false);
    return 0;
}

int main(int argc, char** argv)
{
    if(init_environment(argv[0], argv[1]) < 0)
    {
        std::cerr << "Init failed" << std::endl;
    }

    while(true)
    {

    }
}
