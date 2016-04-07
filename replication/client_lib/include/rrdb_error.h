#pragma once

namespace dsn{ namespace apps {

#define RRDB_ERR_CODE(x, y, z) static const int x = y
#include "rrdb_error_def.h"
#undef RRDB_ERR_CODE

}} //namespace
