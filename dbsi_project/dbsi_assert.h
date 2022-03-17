#ifndef DBSI_ASSERT_H
#define DBSI_ASSERT_H


// always check asserts, except where project logic dictates otherwise
#ifdef NDEBUG
#undef NDEBUG
#define UNDEF_NDEBUG
#endif
#include <cassert>
#ifdef UNDEF_NDEBUG
#define NDEBUG
#undef UNDEF_NDEBUG
#endif


/*
* Dividing assertions into the following groups helps in two
* ways, (i) to document these conditions in the code, and (ii)
* to allow you to enable/disable assertions for different types
* of error independently.
* These checks might be inefficient, but that doesn't matter
* for debugging, as they will be disabled for performance testing.
*/


// #define DBSI_DISABLE_ALL_CHECKS


#ifdef DBSI_DISABLE_ALL_CHECKS
#define DBSI_DISABLE_CHECK_PRECOND
#define DBSI_DISABLE_CHECK_POSTCOND
#define DBSI_DISABLE_CHECK_INVARIANT
#endif


#ifndef DBSI_DISABLE_CHECK_PRECOND
#define DBSI_CHECK_PRECOND(expr) assert(expr)
#define DBSI_CHECKING_PRECONDS
#else
#define DBSI_CHECK_PRECOND(expr) ((void)0)
#endif


#ifndef DBSI_DISABLE_CHECK_POSTCOND
#define DBSI_CHECK_POSTCOND(expr) assert(expr)
#define DBSI_CHECKING_POSTCONDS
#else
#define DBSI_CHECK_POSTCOND(expr) ((void)0)
#endif


#ifndef DBSI_DISABLE_CHECK_INVARIANT
#define DBSI_CHECK_INVARIANT(expr) assert(expr)
#define DBSI_CHECKING_INVARIANTS
#else
#define DBSI_CHECK_INVARIANT(expr) ((void)0)
#endif


#endif  // DBSI_ASSERT_H
