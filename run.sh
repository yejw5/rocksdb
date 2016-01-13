#!/bin/bash

ROOT=`pwd`

if [ -z "$DSN_ROOT" ]; then
    echo "ERROR: DSN_ROOT not defined"
    exit -1
fi

if [ ! -d "$DSN_ROOT" ]; then
    echo "ERROR: DSN_ROOT directory not exist: ${DSN_ROOT}"
    exit -1
fi

function usage()
{
    echo "usage: run.sh <command> [<args>]"
    echo
    echo "Command list:"
    echo "   help           print the help info"
    echo "   build          build the system"
    echo "   start_onebox   start rrdb onebox"
    echo "   stop_onebox    stop rrdb onebox"
    echo "   list_onebox    list rrdb onebox"
    echo "   clear_onebox   clear_rrdb onebox"
    echo "   run_bench      run benchmark test"
    echo
    echo "Command 'run.sh <command> -h' will print help for subcommands."
}

#####################
## build
#####################
function usage_build()
{
    echo "Options for subcommand 'build':"
    echo "   -h|--help         print the help info"
    echo "   -t|--type         build type: debug|release, default is debug"
    echo "   -c|--clear        clear the environment before building"
    echo "   -b|--boost_dir <dir>"
    echo "                     specify customized boost directory,"
    echo "                     if not set, then use the system boost"
    echo "   -w|--warning_all  open all warnings when build, default no"
    echo "   -g|--enable_gcov  generate gcov code coverage report, default no"
    echo "   -v|--verbose      build in verbose mode, default no"
}
function run_build()
{
    BUILD_TYPE="debug"
    CLEAR=NO
    BOOST_DIR=""
    WARNING_ALL=NO
    ENABLE_GCOV=NO
    RUN_VERBOSE=NO
    TEST_MODULE=""
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_build
                exit 0
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift
                ;;
            -c|--clear)
                CLEAR=YES
                ;;
            -b|--boost_dir)
                BOOST_DIR="$2"
                shift
                ;;
            -w|--warning_all)
                WARNING_ALL=YES
                ;;
            -g|--enable_gcov)
                ENABLE_GCOV=YES
                ;;
            -v|--verbose)
                RUN_VERBOSE=YES
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_build
                exit -1
                ;;
        esac
        shift
    done
    if [ "$BUILD_TYPE" != "debug" -a "$BUILD_TYPE" != "release" ]; then
        echo "ERROR: invalid build type \"$BUILD_TYPE\""
        echo
        usage_build
        exit -1
    fi

    cd replication; BUILD_TYPE="$BUILD_TYPE" CLEAR="$CLEAR" \
        BOOST_DIR="$BOOST_DIR" WARNING_ALL="$WARNING_ALL" ENABLE_GCOV="$ENABLE_GCOV" \
        RUN_VERBOSE="$RUN_VERBOSE" TEST_MODULE="$TEST_MODULE" ./build.sh
}

#####################
## start_onebox
#####################
function usage_start_onebox()
{
    echo "Options for subcommand 'start_onebox':"
    echo "   -h|--help         print the help info"
}

function run_start_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_start_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_start_onebox
                exit -1
                ;;
        esac
        shift
    done
    if [ ! -f ${DSN_ROOT}/bin/rrdb/rrdb ]; then
        echo "ERROR: file ${DSN_ROOT}/bin/rrdb/rrdb not exist"
        exit -1
    fi
    run_clear_onebox
    echo "starting server"
    mkdir onebox
    cd onebox
    for i in 1 2 3
    do
        mkdir meta$i;
        cd meta$i
        ln -s ${DSN_ROOT}/bin/rrdb/rrdb rrdb
        ln -s ${ROOT}/replication/config-server.ini config.ini
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
        ln -s ${ROOT}/replication/config-server.ini config.ini
        ./rrdb config.ini -app_list replica@$j &>result &
        PID=$!
        ps -ef | grep rrdb | grep $PID
        cd ..
    done
}

#####################
## stop_onebox
#####################
function usage_stop_onebox()
{
    echo "Options for subcommand 'stop_onebox':"
    echo "   -h|--help         print the help info"
}

function run_stop_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_stop_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_stop_onebox
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep app_list | grep -v grep | awk '{print $2}' | xargs kill &>/dev/null
}

#####################
## list_onebox
#####################
function usage_list_onebox()
{
    echo "Options for subcommand 'list_onebox':"
    echo "   -h|--help         print the help info"
}

function run_list_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_list_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_list_onebox
                exit -1
                ;;
        esac
        shift
    done
    ps -ef | grep rrdb | grep app_list | grep -v grep
}

#####################
## clear_onebox
#####################
function usage_clear_onebox()
{
    echo "Options for subcommand 'clear_onebox':"
    echo "   -h|--help         print the help info"
}

function run_clear_onebox()
{
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_clear_onebox
                exit 0
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_clear_onebox
                exit -1
                ;;
        esac
        shift
    done
    run_stop_onebox
    if [ -d onebox ]; then
        rm -rf onebox
    fi
}

#####################
## run_bench
#####################
function usage_run_bench()
{
    echo "Options for subcommand 'run_bench':"
    echo "   -h|--help            print the help info"
    echo "   -c|--config <path>   config file path, default './replication/config-client.ini'"
    echo "   -t|--type            benchmark type, supporting:"
    echo "                          fillseq_rrdb, fillrandom_rrdb, filluniquerandom_rrdb,"
    echo "                          readrandom_rrdb, deleteseq_rrdb, deleterandom_rrdb"
    echo "                        default is 'fillseq_rrdb,readrandom_rrdb'"
    echo "   -n <num>             number of key/value pairs, default 100000"
    echo "   --thread_num <num>   number of threads, default 1"
    echo "   --key_size <num>     key size, default 16"
    echo "   --value_size <num>   value size, default 100"
    echo "   --timeout <num>      timeout in milliseconds, default 10000"
}

function run_bench()
{
    CONFIG=./replication/config-client.ini
    TYPE=fillseq_rrdb,readrandom_rrdb
    NUM=100000
    THREAD=1
    KEY_SIZE=16
    VALUE_SIZE=100
    TIMEOUT_MS=10000
    while [[ $# > 0 ]]; do
        key="$1"
        case $key in
            -h|--help)
                usage_run_bench
                exit 0
                ;;
            -c|--config)
                CONFIG="$2"
                shift
                ;;
            -t|--type)
                TYPE="$2"
                shift
                ;;
            -n)
                NUM="$2"
                shift
                ;;
            --thread_num)
                THREAD="$2"
                shift
                ;;
            --key_size)
                KEY_SIZE="$2"
                shift
                ;;
            --value_size)
                VALUE_SIZE="$2"
                shift
                ;;
            --timeout)
                TIMEOUT_MS="$2"
                shift
                ;;
            *)
                echo "ERROR: unknown option \"$key\""
                echo
                usage_run_bench
                exit -1
                ;;
        esac
        shift
    done

    ./rrdb_bench --rrdb_config=${CONFIG} --benchmarks=${TYPE} --rrdb_timeout_ms=${TIMEOUT_MS} \
        --key_size=${KEY_SIZE} --value_size=${VALUE_SIZE} --threads=${THREAD} --num=${NUM} \
        --rrdb_app_name=rrdb.instance0 --stats_interval=1000 --histogram=1 \
        --compression_type=none --compression_ratio=1.0
}

####################################################################

if [ $# -eq 0 ]; then
    usage
    exit 0
fi
cmd=$1
case $cmd in
    help)
        usage
        ;;
    build)
        shift
        run_build $*
        ;;
    start_onebox)
        shift
        run_start_onebox $*
        ;;
    stop_onebox)
        shift
        run_stop_onebox $*
        ;;
    clear_onebox)
        shift
        run_clear_onebox $*
        ;;
    list_onebox)
        shift
        run_list_onebox $*
        ;;
    run_bench)
        shift
        run_bench $*
        ;;
    *)
        echo "ERROR: unknown command $cmd"
        echo
        usage
        exit -1
esac

