#!/bin/bash

TMP_DIR=./tmp

sh $DSN_ROOT/bin/dsn.cg.sh rrdb.thrift cpp $TMP_DIR replication
cp -v $TMP_DIR/rrdb.types.h src/
cp -v $TMP_DIR/rrdb.code.definition.h src/
cp -v $TMP_DIR/rrdb.check.h src/
cp -v $TMP_DIR/rrdb.client.h src/
cp -v $TMP_DIR/rrdb.client.perf.h src/
cp -v $TMP_DIR/rrdb.app.example.h src/

rm -rf $TMP_DIR
echo "output directory '$TMP_DIR' deleted"
