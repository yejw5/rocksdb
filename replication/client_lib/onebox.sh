#!/bin/bash

function usage()
{
    echo "usage: run.sh <command> [<args>]"
    echo
    echo "Command list:"
    echo "   help        print the help info"
    echo "   start       start server with 3 metas and 3 replicas"
    echo "   stop        stop server"
    echo "   clear       clear server data"
    echo "   list        list running process"
}


#####################
## list
#####################
function usage_list()
{
    echo "Options for subcommand 'list':"
    echo "   -h|--help         print the help info"
}

function run_list()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_list
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_list
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep app_list | grep -v grep
}

#####################
## clear
#####################
function usage_clear()
{
    echo "Options for subcommand 'clear':"
    echo "   -h|--help         print the help info"
}

function run_clear()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_clear
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_clear
                exit -1
                ;;
        esac
        shift
    done
    if [ -d onebox ]; then
        rm -rf onebox
    fi
}

#####################
## stop
#####################
function usage_stop()
{
    echo "Options for subcommand 'stop':"
    echo "   -h|--help         print the help info"
}

function run_stop()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep app_list | grep -v grep | awk '{print $2}' | xargs kill &>/dev/null
}
#####################
## start
#####################
function usage_start()
{
    echo "Options for subcommand 'start':"
    echo "   -h|--help         print the help info"
}

function run_start()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start
                exit -1
                ;;
        esac
        shift
    done
    run_clear
    echo "starting server"
    mkdir onebox
    cd onebox
    for i in 1 2 3
    do
        mkdir meta$i;
        cd meta$i
        ln -s ${DSN_ROOT}/bin/rrdb/rrdb rrdb
        ln -s ../../config.ini config.ini
        ./rrdb config.ini -app_list meta@$i &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
    done
    for j in 1 2 3
    do
        mkdir replica$j
        cd replica$j
        ln -s ${DSN_ROOT}/bin/rrdb/rrdb rrdb
        ln -s ../../config.ini config.ini
        ./rrdb config.ini -app_list replica@$j &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
    done
}

####################################################################

if [ $# -eq 0 ]; then
    usage
    exit 0
fi
cmd=$1
case $cmd in
    help)
        usage ;;

    start)
        shift
        run_start $*;;

    stop)
        shift
        run_stop $*;;

    clear)
        shift
        run_clear $*;;

    list)
        shift
        run_list $*;;
esac
