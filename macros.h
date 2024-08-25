#pragma once

#define likely(x) __builtin_expect(!!x, 1)
#define unlikely(x) __builtin_expect(!!x, 0)

#define debug 0

#if debug
#include <cassert>
#define DEBUG_ASSERT(x) assert(x)
#else
#define nullify(x) (void)(x)

#define DEBUG_ASSERT(x) nullify(x)

#endif
