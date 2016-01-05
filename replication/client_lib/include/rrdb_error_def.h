ERROR_CODE(ERROR_OK,                    0,      "success");
ERROR_CODE(ERROR_UNKNOWN,               -1,     "unknown error");
ERROR_CODE(ERROR_TIMEOUT,               -2,     "timeout");
ERROR_CODE(ERROR_OBJECT_NOT_FOUND,      -3,     "object not found");

// META SERVER ERROR
ERROR_CODE(ERROR_APP_NOT_EXIST,         -101,   "app not exist");
ERROR_CODE(ERROR_APP_EXIST,             -102,   "app already exist");
ERROR_CODE(ERROR_SERVER_INTERNAL_ERROR, -103,   "server internal error");

// REPLICA SERVER ERROR
ERROR_CODE(ERROR_KEY_NOT_EXIST,         -201,   "key not exist");
ERROR_CODE(ERROR_STORAGE,               -202,   "storage error");

// CLIENT ERROR
ERROR_CODE(ERROR_INVALID_APP_NAME,      -301,   "app name is invalid, only letters, digits or underscore is valid");
ERROR_CODE(ERROR_INVALID_HASH_KEY,      -302,   "hash key can't be empty");
ERROR_CODE(ERROR_INVALID_VALUE,         -303,   "value can't be empty");
ERROR_CODE(ERROR_INVALID_PAR_COUNT,     -304,   "partition count must be a power of 2");
ERROR_CODE(ERROR_INVALID_REP_COUNT,     -305,   "replication count must be 3");
