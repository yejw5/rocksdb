#!/bin/bash

TMP_DIR=./tmp

sh $DSN_ROOT/bin/dsn.cg.sh rrdb.thrift cpp $TMP_DIR replication
cp -v $TMP_DIR/rrdb.types.h src/
cp -v $TMP_DIR/rrdb.code.definition.h src/
cp -v $TMP_DIR/rrdb.check.h src/
cp -v $TMP_DIR/rrdb.server.h src/
cp -v $TMP_DIR/rrdb.client.h src/
cp -v $TMP_DIR/rrdb.client.perf.h src/
cp -v $TMP_DIR/rrdb.app.example.h src/
sed -i 's/# include "rrdb.client.perf.h"/# include "rrdb.client.perf.impl.h"/' src/rrdb.app.example.h
sed -i 's/new rrdb_perf_test_client(/new rrdb_perf_test_client_impl(/' src/rrdb.app.example.h

rm -rf $TMP_DIR
echo "output directory '$TMP_DIR' deleted"
