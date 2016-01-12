#!/bin/sh

PREFIX=rocksdb
if [ $# -eq 1 ]
then
    PREFIX=$1
fi

# config
CONFIG_OUT="${PREFIX}.config"
echo "Generate $CONFIG_OUT"
rm $CONFIG_OUT &>/dev/null
echo "#define __cplusplus 201103L" >>$CONFIG_OUT
echo "#define _DEBUG" >>$CONFIG_OUT
echo "#define ROCKSDB_PLATFORM_POSIX" >>$CONFIG_OUT
echo "#define OS_LINUX" >>$CONFIG_OUT
echo "#define ROCKSDB_FALLOCATE_PRESENT" >>$CONFIG_OUT
echo "#define GFLAGS google" >>$CONFIG_OUT
echo "#define ZLIB" >>$CONFIG_OUT
echo "#define BZIP2" >>$CONFIG_OUT
echo "#define ROCKSDB_MALLOC_USABLE_SIZE" >>$CONFIG_OUT

# includes
INCLUDES_OUT="${PREFIX}.includes"
echo "Generate $INCLUDES_OUT"
rm $INCLUDES_OUT &>/dev/null
echo "." >>$INCLUDES_OUT
echo "include" >>$INCLUDES_OUT
echo "../rDSN/include" >>$INCLUDES_OUT

# files
FILES_OUT="${PREFIX}.files"
echo "Generate $FILES_OUT"
rm $FILES_OUT >&/dev/null
echo "build.sh" >>$FILES_OUT
FILES_DIR="db examples include port replication table tools util utilities"
for i in $FILES_DIR
do
    find $i -name '*.h' -o -name '*.cpp' -o -name '*.c' -o -name '*.cc' \
        -o -name '*.thrift' -o -name '*.ini' \
        -o -name 'CMakeLists.txt' -o -name '*.sh' \
        >>$FILES_OUT
done

