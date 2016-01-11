ERROR_CODE(ERROR_OK,                    0,      "success");
ERROR_CODE(ERROR_UNKNOWN,               -1,     "unknown error");
ERROR_CODE(ERROR_TIMEOUT,               -2,     "timeout");
ERROR_CODE(ERROR_OBJECT_NOT_FOUND,      -3,     "object not found");

// META SERVER ERROR
ERROR_CODE(ERROR_APP_NOT_EXIST,         -101,   "app not exist");
ERROR_CODE(ERROR_APP_EXIST,             -102,   "app already exist");
ERROR_CODE(ERROR_SERVER_INTERNAL_ERROR, -103,   "server internal error");

// CLIENT ERROR
ERROR_CODE(ERROR_INVALID_APP_NAME,      -201,   "app name is invalid, only letters, digits or underscore is valid");
ERROR_CODE(ERROR_INVALID_HASH_KEY,      -202,   "hash key can't be empty");
ERROR_CODE(ERROR_INVALID_VALUE,         -203,   "value can't be empty");
ERROR_CODE(ERROR_INVALID_PAR_COUNT,     -204,   "partition count must be a power of 2");
ERROR_CODE(ERROR_INVALID_REP_COUNT,     -205,   "replication count must be 3");

// SERVER ERROR
// start from -301

// ROCKSDB SERVER ERROR
ERROR_CODE(ERROR_NOT_FOUND,             -1001,  "not found");
ERROR_CODE(ERROR_CORRUPTION,            -1002,  "corruption");
ERROR_CODE(ERROR_NOT_SUPPORTED,         -1003,  "not supported");
ERROR_CODE(ERROR_INVALID_ARGUMENT,      -1004,  "invalid argument");
ERROR_CODE(ERROR_IO_ERROR,              -1005,  "io error");
ERROR_CODE(ERROR_MERGE_IN_PROGRESS,     -1006,  "merge in progress");
ERROR_CODE(ERROR_INCOMPLETE,            -1007,  "incomplete");
ERROR_CODE(ERROR_SHUTDOWN_IN_PROGRESS,  -1008,  "shutdown in progress");
ERROR_CODE(ERROR_ROCKSDB_TIME_OUT,      -1009,  "rocksdb time out");
ERROR_CODE(ERROR_ABORTED,               -1010,  "aborted");
ERROR_CODE(ERROR_BUSY,                  -1011,  "busy");
ERROR_CODE(ERROR_EXPIRED,               -1012,  "expired");
ERROR_CODE(ERROR_TRY_AGAIN,             -1013,  "try again");
