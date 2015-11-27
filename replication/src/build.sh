#!/bin/sh

CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_BUILD_TYPE=Debug"

# identify DSN_ROOT
if [ -z "$DSN_ROOT" ];
then
    DSN_ROOT=`cd ../../../rDSN/builder/output; pwd`
fi
if [ ! -d "$DSN_ROOT" ]
then
    echo "ERROR: DSN_ROOT directory not exist: $DSN_ROOT"
    exit -1
fi
echo "DSN_ROOT=$DSN_ROOT"

# build librocksdb.a
CUR_DIR=`pwd`
cd ../..
make dbg -j8
if [ $? -ne 0 ]
then
    echo "ERROR: build librocksdb.a failed"
    exit -1
else
    echo "Build librocksdb.a succeed"
fi
cd $CUR_DIR

# clear if need
if [ $# -eq 1 -a "$1" == "true" ]
then
    echo "Clear builder..."
    rm -rf builder
    mkdir -p builder
    cd builder
    DSN_ROOT=$DSN_ROOT cmake .. -DCMAKE_INSTALL_PREFIX=`pwd`/output $CMAKE_OPTIONS
    if [ $? -ne 0 ]
    then
        echo "ERROR: cmake failed"
        exit -1
    fi
    cd ..
fi

cd builder
make install -j8
if [ $? -ne 0 ]
then
    echo "ERROR: build failed"
    exit -1
else
    echo "Build succeed"
fi
cd ..

