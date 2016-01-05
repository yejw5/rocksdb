#pragma once

namespace dsn{ namespace apps {

#define ERROR_CODE(x, y, z) static const int x = y
#include "rrdb_error_def.h"
#undef ERROR_CODE

}} //namespace
