#pragma once
#define ORBIT_H

// orbit systems utility header

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <stdalign.h>
#include <stdnoreturn.h>

#ifndef DONT_USE_MARS_ALLOC

#    include "../common/alloc.h"

#    define malloc mars_alloc

#endif

#include "orbit/orbit_types.h"
#include "orbit/orbit_util.h"
#include "orbit/orbit_da.h"
#include "orbit/orbit_ll.h"
#include "orbit/orbit_string.h"
#include "orbit/orbit_fs.h"

#ifndef DONT_USE_MARS_ALLOC
#    undef malloc
#endif
