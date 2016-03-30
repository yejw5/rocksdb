#include <iostream>
#include "info_collector.h"
#include <unistd.h>

using namespace dsn::replication;
using namespace pegasus::apps;

int init_environment(char* exe, char* config_file)
{
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

    info_collector collector;
    while(true)
    {
        collector.on_collect();
        sleep(10);
    }
}
