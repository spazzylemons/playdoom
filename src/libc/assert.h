#ifndef _STUB_ASSERT_H
#define _STUB_ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) ((expr) ? ((void)0) : __builtin_trap())
#endif

#endif
